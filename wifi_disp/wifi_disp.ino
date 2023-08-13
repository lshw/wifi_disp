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
bool power_in = false;
void init1() {
  ht16c21_setup(); //180ms
  ht16c21_cmd(0x88, 1); //闪烁2hz
}
void delay_more() {
  if (power_in) {
    Serial.println(F("外插电,延迟1秒，方便切换"));
    if (millis() < 900)
      delay(1000 - millis());
  }
}
void setup()
{
  Serial.begin(115200);
  load_nvram(); //从esp8266的nvram载入数据
  nvram.boot_count++;
  nvram.change = 1;
  if (nvram.pcb_ver == -1)
    get_value();
  get_batt();
  check_batt_low();
  if (millis() > 5000) { //升级程序后第一次启动
    Serial.println(F("升级完成，重启"));
    nvram.proc = GENERAL_MODE;
    nvram.nvram7 |= NVRAM7_CHARGE;
    nvram.change = 1;
    save_nvram();
    poweroff(2);
  }
  add_limit_millis();
  proc = nvram.proc; //保存当前模式
  _myTicker.attach(1, timer1s);
  switch (proc) { //尽快进行模式切换
    case PROC2_MODE:
      if (power_in)
        nvram.proc = PROC3_MODE; //只有插着电，才可以切换到PROC3
      else
        nvram.proc = OFF_MODE;
      nvram.change = 1;
      save_nvram();
      system_deep_sleep_set_option(1); //重启时校准无线电
      init1();
      disp((char *)" P2 ");
      delay(100);
      delay_more(); //外插电，就多延迟，方便切换
      if (bmp.begin()) {
        snprintf_P(disp_buf, sizeof(disp_buf), PSTR("%f"), bmp.readAltitude());
        disp(disp_buf);
        shan();
        nvram.proc = PROC2_MODE;
        nvram.change = 1;
        save_nvram();
        poweroff(60);
        return;
        break;
      } else {
        nvram.proc = GENERAL_MODE;
        nvram.change = 1;
        save_nvram();
        system_deep_sleep_set_option(2); //重启不校准无线电
        poweroff(2);
      }
      break;
    case PROC3_MODE:
      wifi_set_opmode(STATION_MODE);
      wifi_station_connect();
      if (power_in) { //只有插着电， 才可以换运行模式
        nvram.proc = SETUP_MODE;
        nvram.change = 1;
        save_nvram();
        system_deep_sleep_set_option(1); //下次开机wifi校准
      }
      proc3_setup();
      break;
    case SETUP_MODE:
      wifi_set_opmode(STATIONAP_MODE);
      wifi_station_connect();
      nvram.proc = OFF_MODE;
      nvram.change = 1;
      save_nvram();
      system_deep_sleep_set_option(4); //下次开机关闭wifi
      set_hostname();
      hello();
      setup_setup();
      break;
    case OFF_MODE:
      nvram.proc = LORA_SEND_MODE;
      nvram.change = 1;
      save_nvram();
      system_deep_sleep_set_option(4); //下次开机关闭wifi
      init1();
      set_hostname();
      hello();
      disp((char *)" OFF ");
      delay(2000);
      disp((char *)"-" VER "-");
      delay(2000);
      ht16c21_cmd(0x84, 0x02); //关闭ht16c21
      if (ds_pin == 0) { //v2.0
        if (nvram.have_lora > -5 & lora_init())
          lora.sleep();
        Serial.begin(115200);
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
      if (nvram.have_lora > -5) {
        Serial.println(F("lora  发送模式"));
        nvram.proc = LORA_RECEIVE_MODE;
        nvram.change = 1;
        save_nvram();
        system_deep_sleep_set_option(4); //下次开机关闭wifi
        if (lora_init()) {
          init1();
          disp((char *)"S-" VER);
          wifi_station_disconnect();
          wifi_set_opmode(NULL_MODE);
          WiFi.mode(WIFI_OFF);
          delay(1000);
          return;
        }
      }
    case LORA_RECEIVE_MODE:
      if (nvram.have_lora > -5) {
        Serial.println(F("lora  接收模式"));
        nvram.proc = GENERAL_MODE;
        nvram.change = 1;
        save_nvram();
        system_deep_sleep_set_option(2); //重启时不校准无线电
        init1();
        disp((char *)"L-" VER);
        if (lora_init()) {
          disp((char *)"L-" VER);
          wifi_station_disconnect();
          wifi_set_opmode(NULL_MODE);
          WiFi.mode(WIFI_OFF);
          delay(1000);
          return;
        }
      }
    case GENERAL_MODE:
    default:
      wifi_set_opmode(STATION_MODE);
      wifi_station_connect();
      set_hostname();
      hello();
      proc = GENERAL_MODE;//让后面2个lora在不存在的时候，修正为proc=0
      nvram.proc = PROC2_MODE;
      nvram.change = 1;
      save_nvram();
      system_deep_sleep_set_option(4); //下次开机关闭wifi
      init1();
      Serial.println(F("测温模式"));
      snprintf_P(disp_buf, sizeof(disp_buf), PSTR(" %3.2f "), v);
      disp(disp_buf);
      WiFi_isConnected();
      get_value();
      if (ds_pin == 0 && nvram.have_lora > -5) {
        if (lora_init())
          lora.sleep();
      }
      Serial.println();
      wait_connected(5000);
      shan();
      nvram.proc = GENERAL_MODE;
      system_deep_sleep_set_option(2); //下次开机wifi不校准
      nvram.change = 1;
      save_nvram();
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
        if (nvram.proc != GENERAL_MODE) {
          nvram.proc = GENERAL_MODE;
          nvram.change = 1;
          system_deep_sleep_set_option(2); //重启时不校准无线电
        }
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
    case LORA_RECEIVE_MODE:
      if (ds_pin == 0 && nvram.have_lora > -5) {
        add_limit_millis();
        if (lora_init())
          lora_receive_loop();
        else delay(100);
        break;
      }
    case LORA_SEND_MODE:
      if (ds_pin == 0 && nvram.have_lora > -5) {
        add_limit_millis();
        if (lora_init())
          lora_send_loop();
        delay(400);
        break;
      }
  }
  if (nvram.change) save_nvram();
  yield();
}
