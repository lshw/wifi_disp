#ifndef __GLOBAL_H__
#define __GLOBAL_H__
#include "config.h"
#include "nvram.h"
#include "ht16c21.h"
#include <time.h>
#include "Ticker.h"
#include <ESP8266WiFiMulti.h>
#include <DNSServer.h>
bool get_temp();
Ticker _myTicker;
DNSServer dnsServer;
struct tm now;
int16_t update_timeok = 0; //0-马上wget ，-1 关闭，>0  xx分钟后wget
uint8_t ota_status = 0; //0:wps, 1:ap
uint16_t timer1 = 0; //秒 定时测温
uint16_t timer2 = 0; //秒
uint8_t timer3 = 30; //最长30秒等待上线
void timer1s();
uint8_t proc; //用lcd ram 0 传递过来的变量， 用于通过重启，进行功能切换
#define OTA_MODE 2
#define OFF_MODE 3
#define LORA_RECEIVE_MODE 4
#define LORA_SEND_MODE 5
//0,1-正常 2-OTA 3-off 4-lora接收 5-lora发射

bool wifi_connected_is_ok();
extern bool connected_is_ok;
uint16_t http_get(uint8_t);
bool run_zmd = true;
#define ZMD_BUF_SIZE 100
char zmd_disp[ZMD_BUF_SIZE];
uint8_t zmd_offset = 0, zmd_size = 0;
char disp_buf[22];
extern uint8_t ds_pin ;
extern bool power_in ;
extern bool ap_client_linked ;
uint32_t ap_on_time = 200000;
float get_batt();
float v;
bool power_off = false;
void poweroff(uint32_t sec) {
  get_batt();
  if(nvram.have_dht > -5) {
    void dht_end();
  }
  nvram.proc = 0;
  nvram.change = 1;
  if (ds_pin == 0) Serial.println("V2.0");
  else
    Serial.println("V1.0");
  Serial.println("开机时长:"+String(millis())+"ms");
  if (power_in) Serial.println("有外接电源");
  else Serial.println("无外接电源");
  Serial.flush();
  if (nvram.nvram7 & NVRAM7_CHARGE) {
    if (v > 4.20) {
      Serial.println("v=" + String(v) + ",停止充电");
      nvram.nvram7 &= ~NVRAM7_CHARGE;
      nvram.change = 1;
    }
  }
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  if (power_in && (nvram.nvram7 & NVRAM7_CHARGE)) { //如果外面接了电， 就进入LIGHT_SLEEP模式 电流0.8ma， 保持充电
    sec = sec / 2;
    Serial.print("休眠");
    if (sec > 60) {
      Serial.print(sec / 60);
      Serial.print("分钟");
    }
    if ((sec % 60) != 0) {
      Serial.print(sec % 60);
      Serial.print("秒");
    }
    Serial.println("\r\n充电中");
    Serial.flush();
    wdt_disable();
    uint8_t no_power_in = 0;
    for (uint32_t i = 0; i < sec; i++) {
      yield();
      system_soft_wdt_feed ();
      delay(1000); //空闲时进入LIGHT_SLEEP_T模式
      power_in = i % 2;
      get_batt();
      if (ds_pin != 0) {
        pinMode(13, OUTPUT); //v1.0充电控制
        digitalWrite(13, LOW); //1.0硬件
      } else {
        Serial.end();
        pinMode(1, OUTPUT);
        digitalWrite(1, HIGH); //2.0硬件
      }
      if (!power_in) {
        no_power_in++;
      } else {
        if (no_power_in > 0) no_power_in--;
      }
      if (no_power_in > 5) {
        sec = sec + sec - i; //连续6次测试电源断开
        break;
      }
    }
  }
  if (ds_pin == 0) {
    Serial.begin(115200);
    Serial.println();
  }
  if ((nvram.nvram7 & NVRAM7_CHARGE) && power_in)
    Serial.println("充电结束");
  Serial.print("关机");
  if (sec > 0) {
    if (sec > 60) {
      Serial.print(sec / 60);
      Serial.print("分钟");
    }
    if ((sec % 60) != 0) {
      Serial.print(sec % 60);
      Serial.print("秒");
    }
    Serial.println("\r\nbye!");
  }
  uint64_t sec0 = sec * 1000000;
  Serial.flush();
  if (ds_pin != 0) {
    pinMode(13, OUTPUT);
    digitalWrite(13, HIGH); //v1.0硬件
  } else {
    Serial.println("关闭充电");
    Serial.flush();
    Serial.end();
    pinMode(1, OUTPUT);
    digitalWrite(1, LOW);
  }
  _myTicker.detach();
  wdt_disable();
  system_deep_sleep_set_option(2);
  digitalWrite(LED_BUILTIN, LOW);
  if (sec0 == 0) ht16c21_cmd(0x84, 0x2); //lcd off
  nvram.proc = 0;
  nvram.change = 1;
  save_nvram();
  ESP.deepSleepInstant(sec0, RF_NO_CAL);
  power_off = true;
}
void update_disp() {
  uint8_t zmdsize = strlen(zmd_disp);
  if (connected_is_ok) {
    if (proc == OTA_MODE) {
      snprintf(zmd_disp, sizeof(zmd_disp), " OTA %s -%s-  ", WiFi.localIP().toString().c_str(), VER);
    }
  } else {
    if (proc == OTA_MODE)
      snprintf(zmd_disp, sizeof(zmd_disp), " AP -%s- ", VER);
    else
      snprintf(zmd_disp, sizeof(zmd_disp), " %3.2f -%s-  ", v, VER);
  }
  if (zmdsize != strlen(zmd_disp)) zmd_offset = 0; //长度有变化， 就从头开始显示
}

void timer1s() {
  if (timer3 > 0) {
    if (timer3 == 1) {
      if (nvram.proc != 0) {
        nvram.proc = 0;
        nvram.change = 1;
      }
    }
    timer3--;
  }
  if (timer1 > 0) timer1--;//定时器1 测温
  if (timer2 > 0) timer2--;//定时器2
  //now.tm_sec++;
  //mktime(&now);
  if (proc == OTA_MODE)  {
    if(ota_status == 0  && ap_on_time < millis())
      ap_on_time = millis() + 10000;
    if(!connected_is_ok && ap_on_time > millis()) {
      snprintf(disp_buf, sizeof(disp_buf), "AP%3d", (ap_on_time - millis())/1000);
      if(ota_status == 0){
        disp_buf[0]='P';
        disp_buf[1]='S';
      }
      Serial.begin(115200);
      disp(disp_buf);
      system_soft_wdt_feed ();
      if (power_in == 1) {// 充电控制
        if (ds_pin != 0) digitalWrite(13, HIGH);
        else {
          Serial.flush();
          Serial.end();
          pinMode(1, OUTPUT);
          digitalWrite(1, HIGH);
        }
      }
    } else
      run_zmd = true;
  }
}

uint16_t wget() {
  get_temp();
  uint16_t httpCode = http_get( nvram.nvram7 & NVRAM7_URL); //先试试上次成功的url
  if (httpCode < 200  || httpCode >= 400) {
    nvram.nvram7 = (nvram.nvram7 & ~ NVRAM7_URL) | (~ nvram.nvram7 & NVRAM7_URL);
    nvram.change = 1;
    httpCode = http_get(nvram.nvram7 & NVRAM7_URL); //再试试另一个的url
  }
  return httpCode;
}

float get_batt0() {//锂电池电压
  uint32_t dat = analogRead(A0);
  dat = analogRead(A0)
        + analogRead(A0)
        + analogRead(A0)
        + analogRead(A0)
        + analogRead(A0)
        + analogRead(A0)
        + analogRead(A0)
        + analogRead(A0);

  if (ds_pin != 0) //V1.0硬件分压电阻 499k 97.6k
    v = (float) dat / 8 * (499 + 97.6) / 97.6 / 1023 ;
  else    //V2.0硬件 分压电阻 470k/100k
    v = (float) dat / 8 * (470.0 + 100.0) / 100.0 / 1023 ;
  return v;
}
float get_batt() {
  float v0;
  if (ds_pin != 0) { //v1.0硬件
    pinMode(13, OUTPUT);
    digitalWrite(13, HIGH); //不充电
  } else { //v2.0硬件
    Serial.flush();
    Serial.end();
    pinMode(1, OUTPUT);
    digitalWrite(1, LOW); //不充电
  }
  delay(1);
  get_batt0();
  v0 = v;
  if (ds_pin != 0) //v1.0硬件
    digitalWrite(13, LOW); //充电
  else //v2.0硬件
    digitalWrite(1, HIGH); //充电
  delay(1);
  get_batt0();
  if (v > v0) { //有外接电源
    v0 = v;
    if (ds_pin != 0)
      digitalWrite(13, HIGH); //不充电
    else
      digitalWrite(1, LOW); //不充电
    delay(1);
    get_batt0();
    if (v0 > v) {
      v0 = v;
      if (ds_pin != 0)
        digitalWrite(13, LOW); //充电
      else
        digitalWrite(1, HIGH); //充电
      delay(1);
      get_batt0();
      if (v > v0) {
        v0 = v;
        if (ds_pin != 0)
          digitalWrite(13, HIGH); //不充电
        else
          digitalWrite(1, LOW); //不充电
        delay(1);
        get_batt0();
        if (v0 > v) {
          if (!power_in) {
            power_in = true;
            Serial.println("测得电源插入");
          }
        } else power_in = false;
      } else power_in = false;
    } else power_in = false;
  } else power_in = false;

  if (ds_pin != 0)
    digitalWrite(13, HIGH); //不充电
  else
    digitalWrite(1, LOW); //不充电
  delay(1);
  get_batt0();
  if ((nvram.nvram7 & NVRAM7_CHARGE) == 0) {
    if (v < 3.8) {
      nvram.nvram7 |= NVRAM7_CHARGE;
      nvram.change = 1;
    }
  }
  if (ds_pin == 0) {
    Serial.begin(115200);
  }
  return v;
}

String get_url(uint8_t no) {
  File fp;
  char fn[20];
  String ret;
  if (no == 0 || no == '0') ret = String(DEFAULT_URL0);
  else ret = String(DEFAULT_URL1);
  if (SPIFFS.begin()) {
    if (no == 0 || no == '0')
      fp = SPIFFS.open("/url.txt", "r");
    else
      fp = SPIFFS.open("/url1.txt", "r");
    if (fp) {
      ret = fp.readStringUntil('\n');
      ret.trim();
      fp.close();
      if (ret.startsWith("http://www.cfido.com/")) {
        SPIFFS.remove("/url.txt");
        SPIFFS.remove("/url1.txt");
        ret.replace("www.cfido.com/", "temp.cfido.com:808/");
      } else if (ret.startsWith("http://www.wf163.com/")) {
        SPIFFS.remove("/url.txt");
        SPIFFS.remove("/url1.txt");
        ret.replace("www.wf163.com/", "temp2.wf163.com:808/");
      }
    }
  }
  SPIFFS.end();
  if(ret == ""){
  if(no == 0 || no == '0')
    ret=DEFAULT_URL0;
  else
    ret=DEFAULT_URL1;
  }
  return ret;
}
String get_ssid() {
  File fp;
  String ssid;
  if (SPIFFS.begin()) {
    fp = SPIFFS.open("/ssid.txt", "r");
    if (fp) {
      ssid = fp.readString();
      fp.close();
    } else {
      Serial.println("/ssid.txt open error");
      fp = SPIFFS.open("/ssid.txt", "w");
      ssid = "test:cfido.com";
      fp.println(ssid);
      fp.close();
    }
  } else
    Serial.println("SPIFFS begin error");
  Serial.print("载入ssid设置:");
  Serial.println(ssid);
  SPIFFS.end();
  return ssid;
}

String fp_gets(File fp) {
  int ch = 0;
  String ret = "";
  while (1) {
    ch = fp.read();
    if (ch == -1) return ret;
    if (ch != 0xd && ch != 0xa) break;
  }
  while (ch != -1 && ch != 0xd && ch != 0xa) {
    ret += (char)ch;
    ch = fp.read();
  }
  ret.trim();
  return ret;
}

void zmd() {  //1s 一次Ticker
  uint8_t i = 0, i0 = 0;
  if (!connected_is_ok) return;
  zmd_size = strlen(zmd_disp);
  if (zmd_size == 0) return;
  if (zmd_size < zmd_offset) zmd_offset = 0;
  for (i = 0; i < 10; i++) { //跳过第一个点
    if (zmd_disp[zmd_offset] != '.') break;
    zmd_offset = (zmd_offset + 1) % zmd_size;
  }

  memset(disp_buf, 0, sizeof(disp_buf));
  for (i = 0; i < sizeof(disp_buf); i++) {
    disp_buf[i] = zmd_disp[(zmd_offset + i) % zmd_size];
    if (disp_buf[i] != '.') i0++;
    if (i0 >= 5) break;
  }
  if (disp_buf[1] == '.') { //第一个字母后面是点，就把第一个字母，显示为空格
    for (i = 0; i < sizeof(disp_buf) - 1; i++)
      disp_buf[i] = disp_buf[i + 1];
    disp_buf[0] = ' ';
  }
  disp(disp_buf);
  zmd_offset = (zmd_offset + 1) % zmd_size;
}

#define __YEAR__ ((((__DATE__[7]-'0')*10+(__DATE__[8]-'0'))*10 \
                   +(__DATE__[9]-'0'))*10+(__DATE__[10]-'0'))

#define __MONTH__ (__DATE__[2]=='n'?(__DATE__[1]=='a'?1:6) \   /*Jan:Jun*/
                   :__DATE__[2]=='b'?2 \
                   :__DATE__[2]=='r'?(__DATE__[0]=='M'?3:4) \
                   :__DATE__[2]=='y'?5 \
                   :__DATE__[2]=='l'?7 \
                   :__DATE__[2]=='g'?8 \
                   :__DATE__[2]=='p'?9 \
                   :__DATE__[2]=='t'?10 \
                   :__DATE__[2]=='v'?11:12)

#define __DAY__ ((__DATE__[4]==' '?0:__DATE__[4]-'0')*10 \
                 +(__DATE__[5]-'0'))

void wifi_set_clean() {
  if (SPIFFS.begin()) {
    SPIFFS.remove("/ssid.txt");
    SPIFFS.end();
  }
}
void  wifi_set_add(const char * wps_ssid, const char * wps_password){
  File fp;
  int8_t mh_offset;
  String wifi_sets, line;
  if(wps_ssid[0] == 0) return;
  if (SPIFFS.begin()) {
    fp = SPIFFS.open("/ssid.txt", "r");
    wifi_sets = String(wps_ssid) + ":" + String(wps_password) + "\r\n";
    if (fp) {
      while(fp.available()) {
        line = fp.readStringUntil('\n');
        line.trim();
        if(line == "")
          continue;
        if(line.length() > 110)
          line = line.substring(0, 110);
        mh_offset = line.indexOf(':');
        if(mh_offset < 2) continue;
        if(line.substring(0,mh_offset) == wps_ssid)
          continue;
        else
          wifi_sets += line + "\r\n";
      }
      fp.close();
    }
    fp = SPIFFS.open("/ssid.txt", "w");
    if (fp) {
      fp.print(wifi_sets);
      fp.close();
    }
    SPIFFS.end();
  }
}

#endif
