#include <FS.h>
#ifndef CONFIG_IDF_TARGET_ESP32C3
extern "C" {
#include "user_interface.h"
}
#endif
#include "config.h"
#include "global.h"
bool temp_ok = false; //测温ok
uint32_t temp_start;
void ht16c21_cmd(uint8_t cmd, uint8_t dat);
void init1();
uint32_t next_disp = 1800; //下次开机
String hostname = HOSTNAME;
bool httpd_up = false;

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
  save_nvram();
  ht16c21_setup(); //180ms
  ht16c21_cmd(0x88, 1); //闪烁
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
  uint32_t ms;
  if (millis() > 10000)
    proc = GENERAL_MODE; //程序升级后第一次启动
  Serial.begin(115200);
  load_nvram(); //从esp8266的nvram载入数据
  nvram.boot_count++;
  nvram.change = 1;
  if (nvram.pcb_ver == -1)
    get_value();
  get_batt();
  check_batt_low();
  proc = nvram.proc; //保存当前模式
  switch (proc) { //尽快进行模式切换
    case OTA_MODE:
      nvram.proc = PROC3_MODE;
      nvram.change = 1;
      save_nvram();
      system_deep_sleep_set_option(4); //下次开机关闭wifi
      hello();
      setup_setup();
      _myTicker.attach(1, timer1s);
      break;
    case OFF_MODE:
      wdt_disable();
      nvram.proc = LORA_SEND_MODE;
      system_deep_sleep_set_option(4); //下次开机关闭wifi
      init1();
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
    case LORA_RECEIVE_MODE:
      if (nvram.have_lora > -5) {
        wdt_disable();
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
          delay(1000);
          return;
        }
      }
    case LORA_SEND_MODE:
      if (nvram.have_lora > -5) {
        wdt_disable();
        Serial.println(F("lora  发送模式"));
        nvram.proc = LORA_RECEIVE_MODE;
        system_deep_sleep_set_option(4); //下次开机关闭wifi
        init1();
        disp((char *)"S-" VER);
        if (lora_init()) {
          disp((char *)"S-" VER);
          wifi_station_disconnect();
          wifi_set_opmode(NULL_MODE);
          delay(1000);
          return;
        }
      }
    case PRESSURE_MODE:
      nvram.proc = OTA_MODE;
      system_deep_sleep_set_option(1); //重启时校准无线电
      nvram.change = 1;
      init1();
      disp((char *)"1 PE ");
      delay(100);
      delay_more(); //外插电，就多延迟，方便切换
      if (bmp.begin()) {
        snprintf_P(disp_buf, sizeof(disp_buf), PSTR("%f"), bmp.readAltitude());
        disp(disp_buf);
        nvram.proc = PRESSURE_MODE;
        nvram.change = 1;
        save_nvram();
        poweroff(60);
        return;
        break;
      } else {
        nvram.proc = GENERAL_MODE;
        proc = GENERAL_MODE;
        system_deep_sleep_set_option(2); //重启不校准无线电
        nvram.change = 1;
        init1();
      }
    case GENERAL_MODE:
    default:
      hello();
      proc = GENERAL_MODE;//让后面2个lora在不存在的时候，修正为proc=0
      nvram.proc = PRESSURE_MODE;
      system_deep_sleep_set_option(4); //下次开机关闭wifi
      init1();
      Serial.println(F("测温模式"));
      snprintf_P(disp_buf, sizeof(disp_buf), PSTR(" %3.2f "), v);
      disp(disp_buf);
      WiFi.setAutoConnect(true);//自动链接上次
      wifi_station_connect();
      wifi_setup();
      if (ds_pin == 0 && nvram.have_lora > -5) {
        if (lora_init())
          lora.sleep();
      }
      ms = millis() + 10000;
      while (millis() < ms && !WiFi.isConnected()) {
        yield();
        delay(350);
      }
      get_value();
      set_hostname();
      nvram.proc = GENERAL_MODE;
      system_deep_sleep_set_option(2); //下次开机wifi不校准
      nvram.change = 1;
      break;
  }
}

void wput() {
  ht16c21_cmd(0x88, 1); //开始闪烁
  uint16_t httpCode = wget();
  if (httpCode >= 200 || httpCode < 400) {
    if (v < 3.6)
      ht16c21_cmd(0x88, 2); //0-不闪 1-2hz 2-1hz 3-0.5hz
    else
      ht16c21_cmd(0x88, 0); //0-不闪 1-2hz 2-1hz 3-0.5hz
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

uint32_t last_check_connected;
void loop()
{
  if (power_off) {
    system_soft_wdt_feed ();
    yield();
    return;
  }
  if (proc == OTA_MODE && last_check_connected < millis() &&  wifi_connected_is_ok()) {
    last_check_connected = millis() + 1000; //1秒检查一次connected;
    if ( millis() > ap_on_time && power_in && millis() < 1800000 ) ap_on_time = millis() + 200000; //有外接电源的情况下，最长半小时
    if ( millis() > ap_on_time) { //ap开启时长
      Serial.print(F("batt:"));
      Serial.print(get_batt());
      Serial.print(F("V,millis()="));
      Serial.println(millis());
      Serial.println(F("power down"));
      if (nvram.proc != GENERAL_MODE) {
        nvram.proc = GENERAL_MODE;
        nvram.change = 1;
        system_deep_sleep_set_option(2); //重启时不校准无线电
        save_nvram();
      }
      disp((char *)"00000");
      ht16c21_cmd(0x84, 0);
      httpd.close();
      poweroff(3600);
    }
  }
  switch (proc) {
    case OTA_MODE:
      setup_loop();
      break;
    case LORA_RECEIVE_MODE:
      if (ds_pin == 0 && nvram.have_lora > -5) {
        if (lora_init())
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
      if (wifi_connected_is_ok()) {
        if (!httpd_up ) {
          update_disp();
          wput();
        }
      } else if (millis() > 30000) { //30秒没有连上AP
        if (power_in && smart_config()) {
          disp((char *)"6.6.6.6.6.");
          poweroff(1);
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
      if (ap_client_linked || connected_is_ok) {
        httpd_loop();
      }
  }
  if (nvram.change) save_nvram();
  system_soft_wdt_feed (); //各loop里要根据需要执行喂狗命令
  yield();
}

bool smart_config() {
  //插上电， 等20秒， 如果没有上网成功， 就会进入 CO xx计数， 100秒之内完成下面的操作
  //手机连上2.4G的wifi,然后微信打开网页：http://wx.ai-thinker.com/api/old/wifi/config
  nvram.proc = GENERAL_MODE;
  nvram.change = 1;
  system_deep_sleep_set_option(2); //重启时不校准无线电
  if (wifi_connected_is_ok()) return true;
  WiFi.mode(WIFI_STA);
  WiFi.beginSmartConfig();
  Serial.println(F("SmartConfig start"));
  for (uint8_t i = 0; i < 100; i++) {
    if (WiFi.smartConfigDone()) {
      wifi_set_clean();
      wifi_set_add(WiFi.SSID().c_str(), WiFi.psk().c_str());
      Serial.println(F("OK"));
      return true;
    }
    Serial.write('.');
    delay(1000);
    snprintf_P(disp_buf, sizeof(disp_buf), PSTR("CON%02d"), i);
    disp(disp_buf);
  }
  snprintf_P(disp_buf, sizeof(disp_buf), PSTR(" %3.2f "), v);
  disp(disp_buf);
  return false;
}
