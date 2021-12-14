#include <FS.h>
extern "C" {
#include "user_interface.h"
}
#include "config.h"
#include "global.h"
bool temp_ok = false; //测温ok
uint32_t temp_start;
void ht16c21_cmd(uint8_t cmd, uint8_t dat);
uint32_t next_disp = 1800; //下次开机
String hostname = HOSTNAME;

#include "ota.h"
#include "ds1820.h"
#include "wifi_client.h"
#include "httpd.h"
#include "ht16c21.h"
#include "lora.h"
#ifdef HAVE_DHT
#include "dht.h"
#endif
bool power_in = false;
uint8_t devices = HAVE_DHT | HAVE_LORA;
void setup()
{
  Serial.begin(115200);
  load_nvram(); //从esp8266的nvram载入数据
  nvram.boot_count++;
  save_nvram();
  if (nvram.proc == 0 || nvram.proc ==OTA_MODE ) {
    wifi_setup();
  }
  Serial.println("\r\n\r\n\r\n\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b");
#ifdef GIT_COMMIT_ID
  Serial.println(F("Git Ver=" GIT_COMMIT_ID));
#endif
  Serial.print("Software Ver=" VER "\r\nBuildtime=");
  Serial.print(__YEAR__);
  Serial.write('-');
  if(__MONTH__ < 10) Serial.write('0');
  Serial.print(__MONTH__);
  Serial.write('-');
  if(__DAY__ < 10) Serial.write('0');
  Serial.print(__DAY__);
  Serial.println(F(" " __TIME__));
  Serial.print("proc="); Serial.println(nvram.proc);
  hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(hostname);
  Serial.println("Hostname: " + hostname);
  Serial.flush();
  if (!ds_init() && !ds_init()) ds_init();
#ifdef HAVE_DHT
  if (dht_setup()) {
    devices |= HAVE_DHT;
    devices &= ~HAVE_LORA;
  } else {
    devices &= ~HAVE_DHT;
  }
#endif
  get_temp();
  ht16c21_setup();
  get_batt();
  _myTicker.attach(1, timer1s);
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
      nvram.change = 1; //电压过低
    }
    ht16c21_cmd(0x88, 0); //闪烁
    if (v > 3.45)
      poweroff(7200);//3.45V-3.5V 2小时
    else
      poweroff(3600 * 48); // 低于3.45V 2天
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
        save_nvram(); //4秒之内重启切换功能
      }
      disp(" OFF ");
      delay(2000);
      disp("-" VER "-");
      delay(2000);
      if (nvram.proc != 0) {
        nvram.proc = 0;
        nvram.change = 1; //4秒之后关机， 重启进proc0
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
      httpd_listen();
      ota_setup();
      wdt_disable();
      if (nvram.nvram7 & NVRAM7_CHARGE == 0 || nvram.proc != OFF_MODE) {
        nvram.nvram7 |= NVRAM7_CHARGE; //充电
        nvram.proc = OFF_MODE;//ota以后，
        nvram.change = 1;
        save_nvram();
      }
      disp(" OTA ");
      if (ds_pin == 0) { //v2.0
        if (lora_init())
          lora.sleep();
        Serial.begin(115200);
      }
      ap_on_time = millis() + 200000;
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
      nvram.proc = OTA_MODE;
      nvram.change = 1;
      save_nvram();
      sprintf(disp_buf, " %3.2f ", v);
      disp(disp_buf);
      if (ds_pin == 0) {
        if (lora_init())
          lora.sleep();
        Serial.begin(115200);
        Serial.print("lora version=");
        Serial.println(lora_version);
#ifdef HAVE_DHT
        dht_setup();
#endif
      }
      timer1 = 10;
  }
}

void wput() {
  ht16c21_cmd(0x88, 1); //开始闪烁
  if (nvram.proc != 0) {
    nvram.proc = 0;
    nvram.change = 1;
  }
  if (timer1 > 0) {
    uint16_t httpCode = wget();
    if (httpCode >= 200 || httpCode < 400) {
      if (v < 3.6)
        ht16c21_cmd(0x88, 2); //0-不闪 1-2hz 2-1hz 3-0.5hz
      else
        ht16c21_cmd(0x88, 0); //0-不闪 1-2hz 2-1hz 3-0.5hz
      Serial.print("uptime=");
      Serial.print(millis());
      if (next_disp < 60) next_disp = 1800;
      Serial.print("ms,sleep=");
      Serial.println(next_disp);
      if (millis() < 500) delay(500 - millis());
      save_nvram();
      poweroff(next_disp);
      return;
    } else {
      Serial.print(millis());
      Serial.println("ms,web error,reboot 3600s");
      poweroff(3600);
    }
  }
}

bool httpd_up = false;
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
      httpd_loop();
      ota_loop();
      if (wifi_connected_is_ok()) {
        if (!httpd_up) {
          wput();
          update_disp();
          zmd();
          httpd_up = true;
          httpd_listen();
        }
      } else
        ap_loop();
      break;
    default:
      if (wifi_connected_is_ok()) {
        if (!httpd_up ) {
          update_disp();
          get_temp();
          if (temp[0] < 85.00) {
            wget();
            httpd_up = true;
          }
        }
        return;
      } else if (timer3 == 0) {
        //10秒超时1小时重试。
        Serial.print(millis());
        Serial.println("ms,not link to ap,reboot 3600s");
        if (nvram.proc != 0) {
          nvram.proc = 0;
          nvram.change = 1;
        }
        poweroff(3600);
        return;
      }
  }
  yield();
  if (run_zmd) {
    run_zmd = false;
    zmd();
  }
  if (nvram.change) save_nvram();
  system_soft_wdt_feed (); //各loop里要根据需要执行喂狗命令
}
