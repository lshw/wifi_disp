#include <FS.h>
extern "C" {
#include "user_interface.h"
}
#include "config.h"
#include "global.h"
bool temp_ok = false; //测温ok
extern char ip_buf[30];
uint32_t temp_start;
void ht16c21_cmd(uint8_t cmd, uint8_t dat);
char disp_buf[22];
uint32_t next_disp = 1800; //下次开机
String hostname = HOSTNAME;
uint8_t proc; //用lcd ram 0 传递过来的变量， 用于通过重启，进行功能切换
//0,1-正常 2-OTA 3-off 4-lora接收 5-lora发射

#define OTA_MODE 2
#define OFF_MODE 3
#define LORA_RECEIVE_MODE 4
#define LORA_SEND_MODE 5

#include "ota.h"
#include "ds1820.h"
#include "wifi_client.h"
#include "httpd.h"
#include "ht16c21.h"
#include "lora.h"
#if DHT_HAVE
#include "dht.h"
#endif
bool power_in = false;
void setup()
{
  load_nvram();
  ip_buf[0] = 0;
  Serial.begin(115200);
  Serial.println("\r\n\r\n\r\n\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b");
  Serial.println("Software Ver=" VER "\r\nBuildtime=" __DATE__ " " __TIME__);
  hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(hostname);
  Serial.println("Hostname: " + hostname);
  Serial.flush();
  if (!ds_init() && !ds_init()) ds_init();
  ht16c21_setup();
  get_batt();
  _myTicker.attach(1,timer1s);
  Serial.print("电池电压");
  Serial.println(v);
  if (power_in) {
    Serial.println("外接电源");
  }
  if (v < 3.50 && !power_in) {
    sprintf(disp_buf, "OFF%f", v);
    disp(disp_buf); //电压过低
    if (nvram.nvram7 & NVRAM7_CHARGE == 0 || nvram.proc != 0) {
      nvram.nvram7 |= NVRAM7_CHARGE; //充电
      nvram.proc = 0;
      nvram.change = 1;
    }
    ht16c21_cmd(0x88, 0); //闪烁
    if (v > 3.45)
      poweroff(7200);//3.45V-3.5V 2小时
    else
      poweroff(3600 * 24); //一天
    save_nvram();
    return;
  }
  Serial.flush();
  proc = nvram.proc;
  if (millis() > 10000) proc = 0; //程序升级后第一次启动
  switch (proc) {
    case OFF_MODE: //OFF
      wdt_disable();
      if (nvram.proc != LORA_SEND_MODE) {
        nvram.proc = LORA_SEND_MODE;
        nvram.change = 1;
      }
      disp(" OFF ");
      delay(2000);
      disp("-" VER "-");
      delay(2000);
      if (nvram.proc != 0) {
        nvram.proc = 0;
        nvram.change = 1;
      }
      ht16c21_cmd(0x84, 0x02); //关闭ht16c21
      if (ds_pin == 0) { //v2.0
        if (lora_init())
          lora.sleep();
        Serial.begin(115200);
      }
      save_nvram();
      poweroff(0);
      return;
      break;
    case OTA_MODE:
      wdt_disable();
      if (nvram.nvram7 & NVRAM7_CHARGE == 0 || nvram.proc != OFF_MODE) {
        nvram.nvram7 |= NVRAM7_CHARGE; //充电
        nvram.proc = OFF_MODE;//ota以后，
        nvram.change = 1;
      }
      disp(" OTA ");
      if (ds_pin == 0) { //v2.0
        if (lora_init())
          lora.sleep();
        Serial.begin(115200);
      }
      break;
    case LORA_RECEIVE_MODE:
      if (ds_pin == 0) {
        Serial.println("lora  接收模式");
        wdt_disable();
        if (lora_init()) {
          if (nvram.proc != 0) {
            nvram.proc = 0;
            nvram.change = 1;
          }
          disp("L-" VER);
          wifi_station_disconnect();
          wifi_set_opmode(NULL_MODE);
          save_nvram();
          delay(1000);
          return;
          break;
        }
      }
    case LORA_SEND_MODE:
      if (ds_pin == 0) {
        Serial.println("lora  发送模式");
        wdt_disable();
        if (lora_init()) {
          if (nvram.proc != LORA_RECEIVE_MODE) {
            nvram.proc = LORA_RECEIVE_MODE;
            nvram.change = 1;
          }
          disp("S-" VER);
          wifi_station_disconnect();
          wifi_set_opmode(NULL_MODE);
          save_nvram();
          delay(1000);
          return;
          break;
        }
      }
    default:
      if (nvram.proc != OTA_MODE) {
        nvram.proc = OTA_MODE;
        nvram.change = 1;
      }
      sprintf(disp_buf, " %3.2f ", v);
      disp(disp_buf);
      if (ds_pin == 0) {
        if (lora_init())
          lora.sleep();
        Serial.begin(115200);
        Serial.print("lora version=");
        Serial.println(lora_version);
#if DHT_HAVE
        dht_setup();
#endif
      }
      break;
  }
  //更新时闪烁
  ht16c21_cmd(0x88, 1); //闪烁
  if (wifi_connect() == false) {
    if (proc == OTA_MODE) {
      if (nvram.proc != 0) {
        nvram.proc = 0;
        nvram.change = 1;
        save_nvram();
      }
      ESP.restart();
    }
    if (nvram.proc != 0) {
      nvram.proc = 0;
      nvram.change = 1;
    }
    Serial.print("不能链接到AP\r\n30分钟后再试试\r\n本次上电时长");
    Serial.print(millis());
    Serial.println("ms");
    save_nvram();
    poweroff(1800);
    return;
  }

  if (temp_ok == false) {
    delay(temp_start + 2000 - millis());
    temp_ok = get_temp();
  }
  ht16c21_cmd(0x88, 0); //停止闪烁
  if (proc == OTA_MODE) {
    ota_setup();
    save_nvram();
    return;
  }
  uint16_t httpCode = http_get( nvram.nvram7 & NVRAM7_URL); //先试试上次成功的url
  if (httpCode < 200  || httpCode >= 400) {
    nvram.nvram7 = (nvram.nvram7 & ~ NVRAM7_URL) | (~ nvram.nvram7 & NVRAM7_URL);
    nvram.change = 1;
    httpCode = http_get(nvram.nvram7 & NVRAM7_URL); //再试试另一个的url
  }
  if (httpCode < 200 || httpCode >= 400) {
    Serial.print("不能链接到web\r\n60分钟后再试试\r\n本次上电时长");
    if (nvram.proc != 0) {
      nvram.proc = 0;
      nvram.change = 1;
    }
    Serial.print(millis());
    Serial.println("ms");
    save_nvram();
    poweroff(3600);
    return;
  }
  if (v < 3.6)
    ht16c21_cmd(0x88, 2); //0-不闪 1-2hz 2-1hz 3-0.5hz
  else
    ht16c21_cmd(0x88, 0); //0-不闪 1-2hz 2-1hz 3-0.5hz
  Serial.print("uptime=");
  Serial.print(millis());
  if (next_disp < 60) next_disp = 1800;
  Serial.print("ms,sleep=");
  Serial.println(next_disp);
  save_nvram();
  poweroff(next_disp);
}
void loop()
{
  if (power_off) {
    system_soft_wdt_feed ();
    return;
  }
  switch (proc) {
    case LORA_RECEIVE_MODE:
      if (lora_init())
        lora_receive_loop();
      else delay(400);
      break;
    case LORA_SEND_MODE:
      if (lora_init())
        lora_send_loop();
      delay(400);
      break;
    case OTA_MODE:
      if (WiFiMulti.run() == WL_CONNECTED)
        ota_loop();
      else
        ap_loop();
      break;
  }
  yield();
  if (nvram.change) save_nvram();
  system_soft_wdt_feed (); //各loop里要根据需要执行喂狗命令
}
