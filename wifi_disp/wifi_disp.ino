#include <FS.h>
#define VER "1.47"
#define HOSTNAME "disp_"
extern "C" {
#include "user_interface.h"
}
#include "default.h"
#include "global.h"
bool temp_ok = false; //测温ok
bool lcd_flash = false;
extern char ip_buf[30];
uint32_t temp_start;
void ht16c21_cmd(uint8_t cmd, uint8_t dat);
char disp_buf[22];
uint32_t next_disp = 1800; //下次开机
String hostname = HOSTNAME;
uint8_t proc; //用lcd ram 0 传递过来的变量， 用于通过重启，进行功能切换
//0,1-正常 2-AP 3-OTA  4-http update
#define OTA_MODE 2
#define OFF_MODE 3
#define LORA_RECEIVE_MODE 4
#define LORA_SEND_MODE 5

#include "fs.h"
#include "ota.h"
#include "ds1820.h"
#include "wifi_client.h"
#include "ap_web.h"
#include "ht16c21.h"
#include "http_update.h"
#include "lora.h"
bool power_in = false;
void setup()
{
  uint8_t i;
  ip_buf[0] = 0;
  Serial.begin(115200);
  Serial.println("\r\n\r\n\r\n\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b");
  Serial.println("Software Ver=" VER "\r\nBuildtime=" __DATE__ " " __TIME__);
  hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(hostname);
  Serial.println("Hostname: " + hostname);
  delay(100);
  Serial.flush();
  if (!ds_init() && !ds_init()) ds_init();
  ht16c21_setup();
  get_batt();
  Serial.print("电池电压");
  Serial.println(v);
  if (power_in) {
    Serial.println("外接电源");
  }
  if (v < 3.40 && !power_in) {
    disp(" OFF1"); //电压过低
    ram_buf[7] |= 1; //充电
    ram_buf[0] = 0;
    send_ram();
    poweroff(3600);
    return;
  }
  Serial.flush();
  proc = ram_buf[0];
  if (millis() > 10000) proc = 0; //程序升级后第一次启动
  switch (proc) {
    case OFF_MODE: //OFF
      wdt_disable();

      ram_buf[0] = LORA_SEND_MODE;
      disp(" OFF ");
      delay(2000);
      disp("-" VER "-");
      delay(2000);
      ram_buf[0] = 0;
      send_ram();
      ht16c21_cmd(0x84, 0x02); //关闭ht16c21
      if (ds_pin == 0) { //v2.0
        if (lora_init())
          lora.sleep();
        Serial.begin(115200);
      }
      ram_buf[0] = 0; //自行关闭的话，下一次开机进程序0
      poweroff(0);
      return;
      break;
    case OTA_MODE:
      wdt_disable();
      ram_buf[7] |= 1; //充电
      ram_buf[0] = OFF_MODE;//ota以后，
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
          ram_buf[0] = 0;
          disp("L-" VER);
          wifi_station_disconnect();
          wifi_set_opmode(NULL_MODE);
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
          ram_buf[0] = LORA_RECEIVE_MODE;
          disp("S-" VER);
          send_ram();
          wifi_station_disconnect();
          wifi_set_opmode(NULL_MODE);
          delay(1000);
          return;
          break;
        }
      }
    default:
      ram_buf[0] = OTA_MODE;
      sprintf(disp_buf, " %3.2f ", v);
      disp(disp_buf);
      if (ds_pin == 0) {
        if (lora_init())
          lora.sleep();
        Serial.begin(115200);
        Serial.print(F("lora version="));
        Serial.println(version);
      }
      break;
  }
  send_ram();
  //更新时闪烁
  ht16c21_cmd(0x88, 1); //闪烁

  if (wifi_connect() == false) {
    if (proc == OTA_MODE || proc == OFF_MODE) {
      ram_buf[0] = 0;
      send_ram();
      ESP.restart();
    }
    ram_buf[9] |= 0x10; //x1
    ram_buf[0] = 0;
    send_ram();
    Serial.print("不能链接到AP\r\n30分钟后再试试\r\n本次上电时长");
    Serial.print(millis());
    Serial.println("ms");
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
    return;
  }
  if (proc == OFF_MODE) {
    if (http_update() == true) {
      ram_buf[0] = 0;
      disp("HUP O");
      delay(2000);
    }
    ESP.restart();
    return;
  }
  uint16_t httpCode = http_get((ram_buf[7] >> 1) & 1); //先试试上次成功的url
  if (httpCode < 200  || httpCode >= 400) {
    httpCode = http_get((~ram_buf[7] >> 1) & 1); //再试试另一个的url
  }
  if (httpCode < 200 || httpCode >= 400) {
    SPIFFS.begin();
    Serial.print("不能链接到web\r\n60分钟后再试试\r\n本次上电时长");
    ram_buf[0] = 0;
    send_ram();
    Serial.print(millis());
    Serial.println("ms");
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
  poweroff(next_disp);
}
void loop()
{
  system_soft_wdt_feed ();
  if (power_off) return;
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
}
