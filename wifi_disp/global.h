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
bool upgrading = false;
uint8_t proc; //用lcd ram 0 传递过来的变量， 用于通过重启，进行功能切换
enum {
  NONE_MODE,
  WPS_MODE,
  SMARTCONFIG_MODE
};
uint8_t setup_mode = NONE_MODE;
uint16_t wifi_setup_time = 0;
enum {
  GENERAL_MODE, //0
  PROC2_MODE, //1 P2模式
  PROC3_MODE, //2 P3模式
  SETUP_MODE,//3设置模式
  OFF_MODE,//4关机
  PROC4_MODE, // P3模式
  LORA_RECEIVE_MODE,//lora接收测试
  LORA_SEND_MODE,//lora发送测试
  END_MODE
};
bool WiFi_isConnected();
extern bool connected_is_ok;
uint16_t http_get(uint8_t);
bool ds_init();
bool dht_();
bool sht4x_load();
float sht4x_rh();
float sht4x_temp();
bool lora_init();
extern uint8_t temp_data[6];
extern String hostname;
void httpd_listen();
uint8_t pcb_ver_detect();
void charge_off();
void charge_on();
void lora_sleep();
bool run_zmd = true;
#define ZMD_BUF_SIZE 100
char zmd_disp[ZMD_BUF_SIZE];
uint8_t zmd_offset = 0, zmd_size = 0;
char disp_buf[22];
extern bool power_in ;
extern bool ap_client_linked ;
extern float wendu, shidu;
float get_batt();
float v;
bool Serial_begined = false;
void Serial_begin() {
  if (Serial_begined == false) {
    Serial.begin(115200);
    Serial_begined = true;
  }
}
void Serial_end() {
  if (Serial_begined == true) {
    Serial.flush();
    Serial.end();
    Serial_begined = false;
  }
}
bool power_off = false;
void poweroff(uint32_t sec) {

  if (v >= 3.50) {
    switch_proc_end();
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
  if (power_in && (nvram.nvram7 & NVRAM7_CHARGE)) { //如果外面接了电， 保持充电
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
  if (nvram.pcb_ver == 1) {
    Serial_begin();
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
  digitalWrite(LED_BUILTIN, LOW);
  if (sec0 == 0) ht16c21_cmd(0x84, 0x2); //lcd off
  save_nvram();
  ESP.deepSleep(sec0);
  //  ESP.deepSleepInstant(sec0, RF_NO_CAL);
  power_off = true;
}
void update_disp() {
  uint8_t zmdsize = strlen(zmd_disp);
  if (WiFi.localIP()) {
    snprintf_P(zmd_disp, sizeof(zmd_disp), PSTR(" %s "), WiFi.localIP().toString().c_str(), VER);
  } else {
    snprintf_P(zmd_disp, sizeof(zmd_disp), PSTR(" AP -%s- "), VER);
  }
  if (zmdsize != strlen(zmd_disp)) zmd_offset = 0; //长度有变化， 就从头开始显示
}
uint32_t run_millis_limit = 200000;
void timer1s() {
  if (run_millis_limit < millis()) {
    return; //不喂狗
  }
  system_soft_wdt_feed ();
  if (upgrading)
    return;
  if (setup_mode == NONE_MODE) {
    if (proc == SETUP_MODE)
      run_zmd = true;
    return;
  }
  if (WiFi.localIP()) {
    setup_mode = NONE_MODE;
    return;
  }
  if (wifi_setup_time > 0) {
    if (!power_in && wifi_setup_time > 200)
      wifi_setup_time = 200;
    switch (setup_mode) {
      case WPS_MODE:
        snprintf_P(disp_buf, sizeof(disp_buf), PSTR("PS %02d"), wifi_setup_time % 100);
        break;
      case SMARTCONFIG_MODE:
        snprintf_P(disp_buf, sizeof(disp_buf), PSTR("CO %02d"), wifi_setup_time % 100);
        break;
      default:
        return;
    }
    disp(disp_buf);
    wifi_setup_time --;
  }
  return;
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

  if (nvram.pcb_ver == 0) //V1.0(pvb_ver=0)硬件分压电阻 499k 97.6k
    v = (float) dat / 8 * (499 + 97.6) / 97.6 / 1023 ;
  else    //V2.0,V3.0硬件 分压电阻 470k/100k
    v = (float) dat / 8 * (470.0 + 100.0) / 100.0 / 1023 ;
  return v;
}
float get_batt() {
  if (nvram.pcb_ver == 1) {
    Serial_end(); //pcb1 用TX做充电控制腿
  }
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
  if (nvram.pcb_ver == 1) {
    Serial_begin(); //pcb1 用TX做充电控制腿
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
String build_date() {
  char ymd[sizeof("2023-08-22") + 1 ];
  snprintf_P(ymd, sizeof(ymd), PSTR("%04d-%02d-%02d"), __YEAR__, __MONTH__, __DAY__);
  return String(ymd);
}
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
      Serial_end();
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
      Serial_end();
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
  if (wendu > -300.0) return;
  switch (nvram.pcb_ver) {
    case 2: //sht4x测温湿度, 气压探头
      Serial.println("is pcb2");
      if (sht4x_load()) {
        for (uint8_t i = 0; i < 6; i++)
          Serial.printf_P(PSTR(" %02x"), temp_data[i]);
        Serial.println();
        sht4x_temp();
        sht4x_rh();
        Serial.printf_P(PSTR("温度:%3.1f,湿度:%3.1f%%\r\n"), wendu, shidu);
      } else {
        Serial.println("detect pcb_ver");
        nvram.pcb_ver = -1; //重新诊断pcb_ver
        nvram.change = 1;
      }
      break;
    case 1: //pcb1 ds_pin = 0, have dht, sht4x
      if (sht4x_load()) { //用sht4x
        for (uint8_t i = 0; i < 6; i++)
          Serial.printf_P(PSTR(" %02x"), temp_data[i]);
        Serial.println();
        sht4x_temp();
        sht4x_rh();
        Serial.printf_P(PSTR("温度:%3.1f,湿度:%3.1f%%\r\n"), wendu, shidu);
      } else if (nvram.have_dht > 0 ) { //用dht
        if (!dht_()) {
          nvram.have_dht = 0;
          if (nvram.ds18b20_pin == -2)
            nvram.pcb_ver = -1; //重新诊断pcb_ver
          nvram.change = 1;
        }
      } else if (nvram.ds18b20_pin == 0) {
        pinMode(nvram.ds18b20_pin, INPUT_PULLUP);
        get_temp();
      } else { //无dht 无 ds18b20
        nvram.pcb_ver = -1; //重新诊断pcb_ver
        nvram.change = 1;
      }
      break;
    case 0: //pcb0 only ds_pin = 12;
      if (nvram.ds18b20_pin != 12) {
        nvram.pcb_ver = -1; //重新诊断pcb_ver
        nvram.change = 1;
      } else {
        pinMode(nvram.ds18b20_pin, INPUT_PULLUP);
        get_temp();
      }
      break;
  }
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
      system_deep_sleep_set_option(4); //下次开机关闭wifi
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
bool wait_connected(uint16_t ms) {
  while (millis() < ms && !WiFi_isConnected()) {
    yield();
    Serial.write('.');
    delay(100);
  }
  return WiFi_isConnected();
}
void fix_proc3_set() {
  if (nvram.proc3_host[0] == 0 || nvram.proc3_port < 1024) {
    strncpy(nvram.proc3_host, (char *)"192.168.2.4", sizeof(nvram.proc3_host) - 1);
    nvram.proc3_port = 8888;
    nvram.change = 1;
    save_nvram();
  }
}
void shan() {
  if (v < 3.55)
    ht16c21_cmd(0x88, 2); //0-不闪 1-2hz 2-1hz 3-0.5hz
  else
    ht16c21_cmd(0x88, 0); //0-不闪 1-2hz 2-1hz 3-0.5hz
}

void save_ssid() {
  char wps_ssid[33], wps_password[65];
  delay(1000);
  uint8_t ap_id = wifi_station_get_current_ap_id();
  memset(wps_ssid, 0, sizeof(wps_ssid));
  memset(wps_password, 0, sizeof(wps_password));
  struct station_config config[5];
  wifi_station_get_ap_info(config);
  strncpy(wps_ssid, (char *)config[ap_id].ssid, 32);
  strncpy(wps_password, (char *)config[ap_id].password, 64);
  config[ap_id].bssid_set = 1; //同名ap，mac地址不同
  wifi_station_set_config(&config[ap_id]); //保存成功的ssid,用于下次通讯
  wifi_set_add(wps_ssid, wps_password);
}
bool wifi_config() {
  setup_mode = WPS_MODE;
  wifi_setup_time = 20;
  if (WiFi.beginWPSConfig()) {
    if (WiFi.localIP()) {
      save_ssid();
      setup_mode = NONE_MODE;
      return true;
    }
  }
  setup_mode = SMARTCONFIG_MODE;
  if (power_in)
    wifi_setup_time = 3600;
  else
    wifi_setup_time = 200;
  WiFi.beginSmartConfig();
  return false;
}

void add_limit_millis() {
  if (power_in)
    run_millis_limit = millis() + 3600000; //插着电， 续命3600s
  else
    run_millis_limit = millis() + 200000; //没插电， 续命200s
}
uint8_t pcb_ver_detect() {
  if (nvram.pcb_ver == -1) {
    nvram.have_dht = -1;
    nvram.ds18b20_pin = -1;
  }
  if (nvram.have_dht == -1) {
    if (!dht_())  {//无dht
      Serial.println("have not dht");
      nvram.have_dht = 0;
    } else {
      Serial.println("have dht");
      nvram.have_dht = 1;
      nvram.pcb_ver  = 1; //只有pcb_ver1 才安装dht,
      nvram.ds18b20_pin = -2; //有dht就 无 ds18b20
    }
    nvram.change = 1;
  }
  if (nvram.ds18b20_pin == -1) {
    nvram.ds18b20_pin = 12;
    if (!ds_init()) {
      nvram.ds18b20_pin = 0;
      if (!ds_init()) {
        nvram.ds18b20_pin = -2;
      }
    }
    if (nvram.ds18b20_pin == 0)
      nvram.pcb_ver = 1;
    else if (nvram.ds18b20_pin == 12)
      nvram.pcb_ver = 0;
    nvram.change = 1;
  }
  if (nvram.pcb_ver == -1 && nvram.ds18b20_pin == -2) {
    nvram.pcb_ver = 2; //没有18b20的就是ver2
    nvram.change = 1;
  }
  save_nvram();
  Serial.printf_P(PSTR("detect:pcb_ver =%d\r\n"), nvram.pcb_ver);
  return nvram.pcb_ver;
}
String val_str() {
  float wendu0 = -300.0;
  int32_t qiya = -10e6;
  String msg;
  if (wendu < -299.0 || wendu == 85.0) {
    if (nvram.ds18b20_pin >= 0)
      get_temp();
  }
  if (bmp.begin()) {
    if (wendu > -300.0)
      wendu0 = (wendu + bmp.readTemperature()) / 2;
    else
      wendu0 = bmp.readTemperature();
    qiya = bmp.readPressure();
    snprintf(disp_buf, sizeof(disp_buf), "%f", (float)bmp.readPressure() / 1000);
  } else {
    if (shidu >= 0.0 && shidu <= 100.0)
      snprintf_P(disp_buf, sizeof(disp_buf), PSTR("%02d-%02d"), int(wendu), int(shidu));
    else
      snprintf(disp_buf, sizeof(disp_buf), "%4.2f", wendu);
  }
  disp(disp_buf);
  wendu0 = wendu;
  msg = String(nvram.boot_count) + ',' + String(millis()) + ',' + String(v, 2) + ',';
  if (wendu0 > -41)
    msg += String(wendu0) + ',';
  else
    msg += "-,";
  if (shidu >= 0.0 && shidu <= 100.0)
    msg += String(shidu) + ',';
  else
    msg += "-,";
  if (qiya > -10e6)
    msg += String(qiya);
  else
    msg += '-';
  return msg;
}
void next_wifi_set() { //设置下一次启动时的wifi状态
  switch (nvram.proc) {
    case PROC4_MODE:
      if (!power_in) {
        system_deep_sleep_set_option(4); //下次开机关闭wifi
        break;
      }
    case GENERAL_MODE:
    case PROC3_MODE:
      system_deep_sleep_set_option(2); //下次开机wifi不校准
      break;
    case SETUP_MODE://3设置模式
      system_deep_sleep_set_option(1); //重启时校准无线电
      break;
    case PROC2_MODE: //1 P2模式
    case OFF_MODE://4关机
    case LORA_RECEIVE_MODE://lora接收测试
    case LORA_SEND_MODE://lora发送测试
      system_deep_sleep_set_option(4); //下次开机关闭wifi
  }
}

void switch_proc_begin() { //开始时快速切换
  switch (proc) {
    case PROC4_MODE:
    case LORA_RECEIVE_MODE:
    case LORA_SEND_MODE:
      if (nvram.pcb_ver == 0 || nvram.have_lora == -5 || !lora_init()) {
        //pcb版本0,或者测试了5次都没有lora, 或者测试没有lora,就屏蔽lora功能
        proc = GENERAL_MODE;
        nvram.proc = proc; //无lora
        nvram.change = 1;
        save_nvram();
      }
  }

  if (power_in) { //上电才可以切换
    if (nvram.proc == SETUP_MODE)
      nvram.old_proc = GENERAL_MODE;
    else
      nvram.old_proc = nvram.proc;
    nvram.proc = (nvram.proc + 1) % END_MODE;
  } else {
    if (nvram.proc == OFF_MODE)
      nvram.proc = nvram.old_proc;
    else
      nvram.proc = OFF_MODE;
  }
  nvram.change = 1;
  save_nvram();
  next_wifi_set(); //设置下一次启动时的wifi状态
}
void switch_proc_end() { //关机前设定下次启动的程序
  if (proc == SETUP_MODE)
    nvram.proc = nvram.old_proc;
  nvram.proc = proc; //自然关机就再次启动上次的程序
  nvram.change = 1;
  save_nvram();
  next_wifi_set(); //设置下一次启动时的wifi状态
}
void switch_proc() {
  if (millis() < 1000) //开始1秒钟进行快速切换功能
    switch_proc_begin();
  else
    switch_proc_end(); //关机前设定下次启动的程序
}
#endif
