#include <FS.h>
#ifndef CONFIG_IDF_TARGET_ESP32C3
extern "C" {
#include "user_interface.h"
}
#endif
#include "config.h"
#include "global.h"
void ht16c21_cmd(uint8_t cmd, uint8_t dat);
void init1();
uint32_t next_disp = 1800; //下次开机
String hostname = HOSTNAME;

#include "ota.h"
#include "ds1820.h"
#include "wifi_client.h"
#include "httpd.h"
#include "ht16c21.h"
#include "lora.h"
#include "dht22.h"
#include "sht4x.h"
#include "setup.h"
#include "proc3.h"
#if __has_include("proc4.h")
#include "proc4.h"
#endif
#if __has_include("proc5.h")
#include "proc5.h"
#endif
bool power_in = false;
void init1() {
  ht16c21_setup(); //180ms
  ht16c21_cmd(0x88, 1); //闪烁2hz
}
void setup()
{
  Serial_begin();
  load_nvram(); //从esp8266的nvram载入数据
  nvram.boot_count++;
  nvram.change = 1;
  if (millis() > 5000) { //升级程序后第一次启动
    Serial.println(F("升级完成，重启"));
    nvram.nvram7 |= NVRAM7_CHARGE;
    nvram.change = 1;
    save_nvram();
    poweroff(2);
  }
  if (nvram.have_dht == 0 && nvram.pcb_ver >= 0) { //dht与wifi_station_connect冲突
    if (nvram.proc == PROC3_MODE || nvram.proc == GENERAL_MODE) {
      wifi_set_opmode(STATION_MODE);
      wifi_station_connect();
    } else if (nvram.proc == SETUP_MODE) {
      wifi_set_opmode(STATIONAP_MODE);
      wifi_station_connect();
    }
  } else {
    pcb_ver_detect();
  }
  if (nvram.ds18b20_pin >= 0)
    ds_init();
  get_batt();
  check_batt_low();
  add_limit_millis();
  proc = nvram.proc; //保存当前模式
  switch_proc_begin(); //开始时切换
  switch (proc) { //尽快进行模式切换
#if __has_include("proc5.h")
    case PROC5_MODE:
      proc5_setup();
      break;
#endif
#if __has_include("proc4.h")
    case PROC4_MODE: //lora发送测量值模式, 插电做网关， 不插电做远端.
      proc4_setup();
      break;
#endif
    case PROC2_MODE:
      nvram.change = 1;
      save_nvram();
      init1();
      disp(F(" P2 "));
      delay(100);
      delay_more(); //外插电，就多延迟，方便切换
      if (bmp.begin()) {
        if (nvram.have_bmp != 5) {
          nvram.have_bmp = 5;
          save_nvram();
        }
        snprintf_P(disp_buf, sizeof(disp_buf), PSTR("%f"), bmp.readAltitude());
        disp(disp_buf);
        shan();
        poweroff(60);
        return;
      } else {
        if (nvram.have_bmp > -5)
          nvram.have_bmp--;
        else
          nvram.proc = GENERAL_MODE;
        save_nvram();
        poweroff(2);
      }
      break;
    case PROC3_MODE:
      init1();
      disp(F(" P3 "));
      proc3_setup();
      break;
    case SETUP_MODE:
      set_hostname();
      hello();
      setup_setup();
      break;
    case OFF_MODE:
      init1();
      set_hostname();
      hello();
      disp(F(" OFF "));
      delay(2000);
      disp(F("-" VER "-"));
      delay(2000);
      ht16c21_cmd(0x84, 0x02); //关闭ht16c21
      if (nvram.pcb_ver > 0) { //v2.0
        if (nvram.have_lora > -5 & lora_init())
          lora.sleep();
        Serial_begin();
      }
      if (nvram.have_lora < 0) {
        nvram.have_lora = 0;
        nvram.change = 1;
      }
      save_nvram();
      poweroff(0);
      return;
      break;
    case LORA_SEND_MODE:
      Serial.println(F("lora  发送模式"));
      if (lora_init()) {
        init1();
        disp(F("S-" VER));
        wifi_station_disconnect();
        wifi_set_opmode(NULL_MODE);
        WiFi.mode(WIFI_OFF);
        delay(1000);
        return;
      }
      poweroff(10);
    case LORA_RECEIVE_MODE:
      Serial.println(F("lora  接收模式"));
      init1();
      disp(F("L-" VER));
      if (lora_init()) {
        disp(F("L-" VER));
        wifi_station_disconnect();
        wifi_set_opmode(NULL_MODE);
        WiFi.mode(WIFI_OFF);
        delay(1000);
        return;
      }
      poweroff(10);
    case GENERAL_MODE:
    default:
      proc = GENERAL_MODE;//让后面2个lora在不存在的时候，修正为proc=0
      init1();
      snprintf_P(disp_buf, sizeof(disp_buf), PSTR(" %3.2f "), v);
      disp(disp_buf);
      pcb_ver_detect();
      if (nvram.have_dht == 1 && wendu < -299.0)
        dht_(); //dht必须在wifi打开之前
      wifi_set_opmode(STATION_MODE);
      wifi_station_connect();
      set_hostname();
      hello();
      _myTicker.attach(1, timer1s);
      Serial.println(F("测温模式"));
      if (nvram.have_dht == 0)
        get_value();
      if (wendu == 85.00 && nvram.ds18b20_pin >= 0) {
        for (uint8_t i = 0 ; i < 10; i++) {
          if (get_temp()) {
            Serial.printf_P(PSTR("get_temp() %d\r\n"), i);
            break;
          }
          yield();
          delay(100);
        }
      }
      WiFi_isConnected();
      if (nvram.pcb_ver > 0 && nvram.have_lora > -5) {
        if (lora_init())
          lora.sleep();
      }
      Serial.println();
      wait_connected(5000);
      shan();
      wait_connected(30000);
      Serial.println();
      if (WiFi_isConnected()) {
        wput();
      } else { //30秒没有连上AP
        if (power_in) {
          wifi_config();
          return;
        }
        //10秒超时1小时重试。
        Serial.print(millis());
        Serial.println(F("ms,not link to ap,reboot 3600s"));
        ht16c21_cmd(0x88, 3); //慢闪烁
        poweroff(3600);
      }
      break;
  }
}

void wput() {
  uint16_t httpCode = wget();
  if (httpCode >= 200 || httpCode < 400) {
    Serial.print(F("uptime="));
    Serial.print(millis());
    if (next_disp < 60) next_disp = 1800;
    Serial.print(F("ms,sleep="));
    Serial.println(next_disp);
    if (millis() < 500) delay(500 - millis());
    poweroff(next_disp);
  } else {
    Serial.print(millis());
    Serial.println(F("ms,web error,reboot 3600s"));
    ht16c21_cmd(0x88, 3); //慢闪烁
    poweroff(3600);
  }
}

void loop()
{
  system_soft_wdt_feed ();
  if (power_off) {
    yield();
    delay(1000);
    return;
  }
  if (run_millis_limit <  millis()) {
    Serial.print(F("超时关机，millis()="));
    Serial.println(millis());
    poweroff(3600);
  }
  switch (proc) {
    case SETUP_MODE:
      setup_loop();
      break;
    case PROC4_MODE:
      if (nvram.pcb_ver > 0 && nvram.have_lora > -5) {
        add_limit_millis();
        if (lora_init()) {
          lora_receive_proc4();
          if (rxLen > 0 && send_limit < 80) {
            wget();
            rxLen = 0;
            rxBuf[0] = 0;
            send_limit += 20; //猝发4次后每20秒发送一次
          }
        } else delay(200);
        break;
      }
    case LORA_RECEIVE_MODE:
      if (nvram.pcb_ver > 0 && nvram.have_lora > -5) {
        add_limit_millis();
        if (lora_init())
          lora_receive_loop();
        else delay(200);
        break;
      }
    case LORA_SEND_MODE:
      if (nvram.pcb_ver > 0 && nvram.have_lora > -5) {
        add_limit_millis();
        if (lora_init())
          lora_send_loop();
        delay(400);
        break;
      }
  }

  if (setup_mode == SMARTCONFIG_MODE) {
    if (WiFi.smartConfigDone()) {
      yield();
      save_ssid();
      setup_mode = NONE_MODE;
      WiFi.stopSmartConfig();
    }
    setup_loop();
  }
  if (nvram.change) save_nvram();
  get_batt();
  yield();
  if (proc == SETUP_MODE && nvram.pcb_ver == 1) {
    Serial_end();
    charge_on();
    delay(350);
    charge_off();
    Serial_begin();
  } else
    delay(350);
  yield();
}
