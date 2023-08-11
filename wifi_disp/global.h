#ifndef __GLOBAL_H__
#define __GLOBAL_H__
#include "config.h"
#include "nvram.h"
#include "ht16c21.h"
#include <time.h>
#include "Ticker.h"
#ifdef CONFIG_IDF_TARGET_ESP32C3
#include <WiFiMulti.h>
#include <WiFiUdp.h>
#define  LIGHT_SLEEP_T ESP_LIGHT_SLEEP
#define wdt_disable() rtc_wdt_disable()
#else
#include <ESP8266WiFiMulti.h>
#endif
#include <DNSServer.h>
#include "CRC32.h"
CRC32 crc;
#include <Adafruit_BMP085.h>
Adafruit_BMP085 bmp;
bool get_temp();
Ticker _myTicker;
DNSServer dnsServer;
struct tm now;
bool upgrading = false;
int16_t update_timeok = 0; //0-马上wget ，-1 关闭，>0  xx分钟后wget
uint8_t ota_status = 0; //0:wps, 1:ap
void timer1s();
uint8_t proc; //用lcd ram 0 传递过来的变量， 用于通过重启，进行功能切换
enum {
  GENERAL_MODE, //0
  PRESSURE_MODE, //1
  SETUP_MODE,//2设置模式
  PROC3_MODE, //3P3模式
  OFF_MODE,//4关机
  LORA_RECEIVE_MODE,//5lora接收测试
  LORA_SEND_MODE//6lora发送测试
};
bool WiFi_isConnected();
extern bool connected_is_ok;
uint16_t http_get(uint8_t);
bool ds_init();
bool dht();
bool sht4x_load();
float sht4x_rh();
float sht4x_temp();
extern uint8_t temp_data[6];
extern String hostname;
void httpd_listen();
void charge_off();
void charge_on();
void lora_sleep();
bool run_zmd = true;
#define ZMD_BUF_SIZE 100
char zmd_disp[ZMD_BUF_SIZE];
uint8_t zmd_offset = 0, zmd_size = 0;
char disp_buf[22];
extern uint8_t ds_pin ;
extern bool power_in ;
extern bool ap_client_linked ;
extern float wendu, shidu;
uint32_t ap_on_time = 200000;
float get_batt();
float v;
bool power_off = false;
void udp_send(String msg, char * dst_host, uint16_t dst_port, uint16_t src_port) {
  WiFiUDP udp;
  udp.begin(src_port);
  udp.beginPacket(dst_host, dst_port);
  udp.println(msg);
  udp.endPacket();
}
void poweroff(uint32_t sec) {
  if (nvram.proc != PRESSURE_MODE && nvram.proc != PROC3_MODE) {
    nvram.proc = GENERAL_MODE;
    nvram.change = 1;
  }

  get_batt();
  Serial.printf_P(PSTR("开机时长:%ld ms\r\n"), (uint32_t)millis());
  if (power_in) Serial.println(F("有外接电源"));
  else Serial.println(F("无外接电源"));
  Serial.flush();
  if (nvram.nvram7 & NVRAM7_CHARGE) {
    if (v > 4.20) {
      Serial.printf_P(PSTR("v=%f,停止充电"), v);
      nvram.nvram7 &= ~NVRAM7_CHARGE;
      nvram.change = 1;
    }
  }
  if (power_in && (nvram.nvram7 & NVRAM7_CHARGE) && nvram.proc != PROC3_MODE) { //如果外面接了电， 保持充电
    sec = sec / 2;
    Serial.print(F("休眠"));
    if (sec > 60) {
      Serial.print(sec / 60);
      Serial.print(F("分钟"));
    }
    if ((sec % 60) != 0) {
      Serial.print(sec % 60);
      Serial.print(F("秒"));
    }
    Serial.println(F("\r\n充电中"));
    Serial.flush();
    uint8_t no_power_in = 0;
    for (uint32_t i = 0; i < sec; i++) {
      yield();
      //system_soft_wdt_feed ();
      delay(1000);
      power_in = i % 2;
      get_batt();
      charge_on();
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
    Serial.println(F("充电结束"));
  Serial.print(F("关机"));
  if (sec > 0) {
    if (sec > 60) {
      Serial.print(sec / 60);
      Serial.print(F("分钟"));
    }
    if ((sec % 60) != 0) {
      Serial.print(sec % 60);
      Serial.print(F("秒"));
    }
    Serial.println(F("\r\nbye!"));
  }
  uint64_t sec0 = sec * 1000000;
  Serial.flush();
  charge_off();
  _myTicker.detach();
  wdt_disable();
  if (nvram.proc ==  PRESSURE_MODE || nvram.proc == PROC3_MODE)
    system_deep_sleep_set_option(4); //下次开机关闭wifi
  else
    system_deep_sleep_set_option(2); //下次启动不做无线电校准
  digitalWrite(LED_BUILTIN, LOW);
  if (sec0 == 0) ht16c21_cmd(0x84, 0x2); //lcd off
  save_nvram();
  ESP.deepSleepInstant(sec0, RF_NO_CAL);
  power_off = true;
}
void update_disp() {
  uint8_t zmdsize = strlen(zmd_disp);
  if (connected_is_ok) {
    if (proc == SETUP_MODE) {
      snprintf_P(zmd_disp, sizeof(zmd_disp), PSTR(" OTA %s -%s-  "), WiFi.localIP().toString().c_str(), VER);
    }
  } else {
    if (proc == SETUP_MODE)
      snprintf_P(zmd_disp, sizeof(zmd_disp), PSTR(" AP -%s- "), VER);
    else
      snprintf_P(zmd_disp, sizeof(zmd_disp), PSTR(" %3.2f -%s-  "), v, VER);
  }
  if (zmdsize != strlen(zmd_disp)) zmd_offset = 0; //长度有变化， 就从头开始显示
}

void timer1s() {
  if (upgrading)
    return;
  if (ota_status == 0  && ap_on_time < millis())
    ap_on_time = millis() + 10000;
  if (!connected_is_ok && ap_on_time > millis()) {
    snprintf_P(disp_buf, sizeof(disp_buf), PSTR("AP%3d"), (ap_on_time - millis()) / 1000);
    if (ota_status == 0) {
      disp_buf[0] = 'P';
      disp_buf[1] = 'S';
    }
    Serial.begin(115200);
    disp(disp_buf);
    //system_soft_wdt_feed ();
    if (power_in == 1) {// 充电控制
      charge_on();
    }
  } else
    run_zmd = true;
}

uint16_t wget() {
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
  charge_off();
  delay(1);
  if (get_batt0() < 1.0) //电压低于1.0v但是还能运行，使用的是外接电源
    power_in = true;
  else {
    float v0;
    v0 = v; //不充电时的电压
    charge_on();
    delay(1);
    get_batt0();
    if (v > v0) { //有外接电源
      v0 = v;
      charge_off();
      delay(1);
      get_batt0();
      if (v0 > v) {
        v0 = v;
        charge_on();
        delay(1);
        get_batt0();
        if (v > v0) {
          v0 = v;
          charge_off();
          delay(1);
          get_batt0();
          if (v0 > v) {
            if (!power_in) {
              power_in = true;
            }
          } else power_in = false;
        } else power_in = false;
      } else power_in = false;
    } else power_in = false;
  }
  charge_off();
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
  Serial.printf_P(PSTR("电池电压%.2f\r\n"), v);
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
  if (ret == "") {
    if (no == 0 || no == '0')
      ret = DEFAULT_URL0;
    else
      ret = DEFAULT_URL1;
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
      Serial.println(F("/ssid.txt open error"));
      fp = SPIFFS.open("/ssid.txt", "w");
      ssid = "test:cfido.com";
      fp.println(ssid);
      fp.close();
    }
  } else
    Serial.println(F("SPIFFS begin error"));
  Serial.print(F("载入ssid设置:"));
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

#define __MONTH__ (__DATE__[2]=='n'?(__DATE__[1]=='a'?1:6)  /*Jan:Jun*/ \
                   : __DATE__[2] == 'b' ? 2 \
                   : __DATE__[2] == 'r' ? (__DATE__[0] == 'M' ? 3 : 4) \
                   : __DATE__[2] == 'y' ? 5 \
                   : __DATE__[2] == 'l' ? 7 \
                   : __DATE__[2] == 'g' ? 8 \
                   : __DATE__[2] == 'p' ? 9 \
                   : __DATE__[2] == 't' ? 10 \
                   : __DATE__[2] == 'v' ? 11 : 12)

#define __DAY__ ((__DATE__[4]==' '?0:__DATE__[4]-'0')*10 \
                 +(__DATE__[5]-'0'))

void wifi_set_clean() {
  if (SPIFFS.begin()) {
    SPIFFS.remove("/ssid.txt");
    SPIFFS.end();
  }
}
void  wifi_set_add(const char * wps_ssid, const char * wps_password) {
  File fp;
  int8_t mh_offset;
  String wifi_sets, line;
  if (wps_ssid[0] == 0) return;
  if (SPIFFS.begin()) {
    fp = SPIFFS.open("/ssid.txt", "r");
    wifi_sets = String(wps_ssid) + ":" + String(wps_password) + "\r\n";
    if (fp) {
      while (fp.available()) {
        line = fp.readStringUntil('\n');
        line.trim();
        if (line == "")
          continue;
        if (line.length() > 110)
          line = line.substring(0, 110);
        mh_offset = line.indexOf(':');
        if (mh_offset < 2) continue;
        if (line.substring(0, mh_offset) == wps_ssid)
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
void charge_on() {
  switch (nvram.pcb_ver) {
    case 0: //pcb0
      pinMode(13, OUTPUT);
      digitalWrite(13, LOW);
      break;
    default:
    case 1: //pcb1
      Serial.flush();
      Serial.end();
      pinMode(1, OUTPUT);
      digitalWrite(1, HIGH);
      break;
    case 2: //pcb2
      pinMode(15, OUTPUT);
      digitalWrite(15, HIGH);
      break;
  }
}
void charge_off() {
  switch (nvram.pcb_ver) {
    case 0: //pcb0
      pinMode(13, OUTPUT);
      digitalWrite(13, HIGH);
      break;
    default:
    case 1: //pcb1
      Serial.flush();
      Serial.end();
      pinMode(1, OUTPUT);
      digitalWrite(1, LOW);
      break;
    case 2: //pcb2
      pinMode(15, OUTPUT);
      digitalWrite(15, LOW);
      break;
  }
}
void set_hostname() {
  hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(hostname);
}
void hello() {
#ifdef GIT_VER
  Serial.println(F("Git Ver=" GIT_VER));
#endif
  Serial.print(F("SDK Ver="));
  Serial.println(ESP.getSdkVersion());
  Serial.printf_P(PSTR("GCC%d.%d\r\n"
                       "Software Ver=" VER "\r\n"
                       "Buildtime=%d-%02d-%02d " __TIME__ "\r\n"
                       "build_set:[" BUILD_SET "]\r\n"), __GNUC__, __GNUC_MINOR__,
                  __YEAR__, __MONTH__, __DAY__);
  Serial.println(F("Hostname: ") + hostname);
  Serial.flush();
}
void get_value() {
  if (sht4x_load()) {
    pinMode(0, INPUT_PULLUP);
    nvram.pcb_ver = 2;
    nvram.change = 1;
    ds_pin = 0;//DHT22使用V2.0的硬件
    for (uint8_t i = 0; i < 6; i++)
      Serial.printf_P(PSTR(" %02x"), temp_data[i]);
    Serial.println();
    sht4x_temp();
    sht4x_rh();
    Serial.printf_P(PSTR("温度:%3.1f,湿度:%3.1f%%\r\n"), wendu, shidu);
  } else {
    if (nvram.have_dht > 0 ) {
      if (!dht() &&  !dht()) {
        nvram.have_dht = 0;
        nvram.change = 1;
      }
    }
    if (nvram.have_dht <= 0) {
      if (!ds_init()  && !ds_init()) {
        nvram.have_dht = 1;
        nvram.change = 1;
      }
    }
    if (nvram.have_dht <= 0)
      get_temp();
    else {
      pinMode(0, INPUT_PULLUP);
      ds_pin = 0;//DHT22使用V2.0的硬件
    }
    if (ds_pin == 0)
      nvram.pcb_ver = 1;
    else
      nvram.pcb_ver = 0;
    nvram.change = 1;
  }
  Serial.printf_P(PSTR("pcb ver = %d\r\n"), nvram.pcb_ver);
}
void check_batt_low() {
  if (power_in) {
    Serial.println(F("有外接电源"));
  } else if (v < 3.50) {
    snprintf_P(disp_buf, sizeof(disp_buf), PSTR("OFF%f"), v);
    disp(disp_buf); //电压过低
    if (nvram.nvram7 & NVRAM7_CHARGE == 0 || nvram.proc != 0) {
      nvram.nvram7 |= NVRAM7_CHARGE; //充电
      nvram.proc = GENERAL_MODE;
      system_deep_sleep_set_option(2); //重启时不校准无线电
      nvram.change = 1; //电压过低
    }
    ht16c21_cmd(0x88, 0); //闪烁
    if (v > 3.45)
      poweroff(7200);//3.45V-3.5V 2小时
    else
      poweroff(3600 * 48); // 低于3.45V 2天
    return;
  }
}
void wait_connected(uint16_t ms) {
  while (millis() < ms && !WiFi_isConnected()) {
    //system_soft_wdt_feed ();
    yield();
    Serial.write('.');
    delay(100);
  }
}
#endif
