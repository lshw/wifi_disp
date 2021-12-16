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
#include "dht.h"
bool power_in = false;
void init1(){
  save_nvram();
  ht16c21_setup(); //180ms
  ht16c21_cmd(0x88, 1); //闪烁
}
void setup()
{
  Serial.begin(115200);
  load_nvram(); //从esp8266的nvram载入数据
  nvram.boot_count++;
  nvram.change = 1;
  proc = nvram.proc; //保存当前模式
  switch(proc) { //尽快进行模式切换
    case OTA_MODE:
      nvram.proc = OFF_MODE;
      init1();
      disp(" OTA ");
      wifi_setup();
      break;
    case OFF_MODE:
      nvram.proc = LORA_SEND_MODE;
      init1();
      disp(" OFF ");
      break;
    case LORA_SEND_MODE:
      if(nvram.have_lora > -5) {
        nvram.proc = LORA_RECEIVE_MODE;
        init1();
        disp("S-" VER);
        break;
      }
    case LORA_RECEIVE_MODE:
      if(nvram.have_lora > -5) {
        nvram.proc = 0;
        init1();
        disp("L-" VER);
        break;
      }
    default:
      nvram.proc = OTA_MODE;
      init1();
      wifi_setup();
      break;
  }

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
  hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(hostname);
  Serial.println("Hostname: " + hostname);
  Serial.flush();
  if (!ds_init() && !ds_init()) ds_init();
  
  if (nvram.have_dht > -5 && dht_setup()) {
    if(nvram.have_dht < 1) {
      nvram.have_dht = 1;
      nvram.change = 1;
    }
    if(nvram.have_lora > -5) {
      nvram.have_lora = -5;
      nvram.change = 1;
    }
  } else {
    if(nvram.have_dht > -5) {
      nvram.have_dht --;
      nvram.change = 1;
    }
  }
  get_temp();
  get_batt();
  _myTicker.attach(1, timer1s);
  Serial.print("电池电压");
  Serial.println(v);
  if (power_in) {
    Serial.println("外接电源");
  }
  if (v < 3.50 && !power_in) {
    snprintf(disp_buf, sizeof(disp_buf), "OFF%f", v);
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
  if (millis() > 10000) proc = 0; //程序升级后第一次启动
  switch (proc) {
    case OFF_MODE: //OFF
      wdt_disable();
      disp(" OFF ");
      delay(2000);
      disp("-" VER "-");
      delay(2000);
      ht16c21_cmd(0x84, 0x02); //关闭ht16c21
      if (ds_pin == 0) { //v2.0
        if (nvram.have_lora > -5 & lora_init())
          lora.sleep();
        Serial.begin(115200);
      }
      if(nvram.have_dht < 0 || nvram.have_lora < 0) {
        nvram.have_dht = 0;  //清除无dht和lora 的标志， 重新诊断
        nvram.have_lora = 0;
        nvram.change = 1;
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
        if (nvram.have_lora > -5 && lora_init())
          lora.sleep();
        Serial.begin(115200);
      }
      ap_on_time = millis() + 200000;
      break;
    case LORA_RECEIVE_MODE:
      if (ds_pin == 0 && nvram.have_lora > -5) {
        Serial.println("lora  接收模式");
        wdt_disable();
        if (lora_init()) {
          disp("L-" VER);
          wifi_station_disconnect();
          wifi_set_opmode(NULL_MODE);
          delay(1000);
          return;
          break;
        }
      }
    case LORA_SEND_MODE:
      if (ds_pin == 0 && nvram.have_lora > -5) {
        Serial.println("lora  发送模式");
        wdt_disable();
        if (lora_init()) {
          disp("S-" VER);
          wifi_station_disconnect();
          wifi_set_opmode(NULL_MODE);
          delay(1000);
          return;
          break;
        }
      }
    default:
      Serial.println("测温模式");
      snprintf(disp_buf, sizeof(disp_buf), " %3.2f ", v);
      disp(disp_buf);
      if (ds_pin == 0 && nvram.have_lora > -5) {
        if (lora_init())
          lora.sleep();
        Serial.begin(115200);
        Serial.print("lora version=");
        Serial.println(lora_version);
      }
      timer1 = 10;
  }
}

void wput() {
  ht16c21_cmd(0x88, 1); //开始闪烁
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
      ht16c21_cmd(0x88,3); //慢闪烁
      poweroff(3600);
    }
  }
}

bool httpd_up = false;
uint32_t last_check_connected;
void loop()
{
  if(last_check_connected < millis() &&  wifi_connected_is_ok()) {
    last_check_connected = millis() + 1000; //1秒检查一次connected;
  }
  if (power_off) {
    system_soft_wdt_feed ();
    return;
  }
  switch (proc) {
    case OTA_MODE:
      httpd_loop();
      ota_loop();
      if (connected_is_ok) {
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
    case LORA_RECEIVE_MODE:
      if (ds_pin == 0 && nvram.have_lora > -5) {
        if(lora_init())
          lora_receive_loop();
        else delay(100);
      break;
      }
    case LORA_SEND_MODE:
      if (ds_pin == 0 && nvram.have_lora > -5) {
        if (lora_init())
          lora_send_loop();
        delay(400);
        break;
      }
    default:
      if (connected_is_ok) {
        if (!httpd_up ) {
          update_disp();
          get_temp();
          if (temp[0] < 85.00) {
            wput();
            httpd_up = true;
          }
        }
        return;
      } else if (timer3 == 0) {
        //10秒超时1小时重试。
        Serial.print(millis());
        Serial.println("ms,not link to ap,reboot 3600s");
        ht16c21_cmd(0x88,3); //慢闪烁
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
  if(nvram.proc != 0 && millis() > 5000){//5秒后， 如果重启， 就测温
    nvram.proc = 0;
    nvram.change = 1;
  }
  if (nvram.change) save_nvram();
  system_soft_wdt_feed (); //各loop里要根据需要执行喂狗命令
}
