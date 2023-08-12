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
  Serial.begin(115200);
  load_nvram(); //从esp8266的nvram载入数据
  nvram.boot_count++;
  nvram.change = 1;
  if (nvram.pcb_ver == -1)
    get_value();
  get_batt();
  check_batt_low();
  if (millis() > 5000) { //升级程序后第一次启动
    Serial.println("升级完成，重启");
    nvram.proc = GENERAL_MODE;
    nvram.nvram7 |= NVRAM7_CHARGE;
    nvram.change = 1;
    save_nvram();
    poweroff(2);
  }
  proc = nvram.proc; //保存当前模式
  wdt_disable();
  switch (proc) { //尽快进行模式切换
    case PRESSURE_MODE:
      nvram.proc = SETUP_MODE;
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
        nvram.proc = PRESSURE_MODE;
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
    case SETUP_MODE:
      WiFi.setAutoConnect(true);//自动链接上次
      wifi_station_connect();
      WiFi.mode(WIFI_STA);
      if (power_in)
        nvram.proc = PROC3_MODE; //只有插着电，才可以切换到PROC3
      else
        nvram.proc = OFF_MODE;
      nvram.change = 1;
      save_nvram();
      system_deep_sleep_set_option(4); //下次开机关闭wifi
      set_hostname();
      hello();
      setup_setup();
      _myTicker.attach(1, timer1s);
      break;
    case PROC3_MODE:
      WiFi.setAutoConnect(true);//自动链接上次
      wifi_station_connect();
      WiFi.mode(WIFI_STA);
      if (power_in) { //只有插着电， 才可以换运行模式
        nvram.proc = OFF_MODE;
        nvram.change = 1;
        save_nvram();
        system_deep_sleep_set_option(4); //下次开机关闭wifi
      }
      proc3_setup();
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
        init1();
        disp((char *)"S-" VER);
        if (lora_init()) {
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
      WiFi.mode(WIFI_STA);
      set_hostname();
      WiFi.setAutoConnect(true);//自动链接上次
      wifi_station_connect();
      hello();
      proc = GENERAL_MODE;//让后面2个lora在不存在的时候，修正为proc=0
      nvram.proc = PRESSURE_MODE;
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
      nvram.proc = GENERAL_MODE;
      system_deep_sleep_set_option(2); //下次开机wifi不校准
      nvram.change = 1;
      save_nvram();
      wait_connected(30000);
      Serial.println();
      if (WiFi_isConnected()) {
        wput();
      } else { //30秒没有连上AP
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
    delay(1000);
    return;
  }
  if (proc == SETUP_MODE && last_check_connected < millis() &&  WiFi_isConnected()) {
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
    case SETUP_MODE:
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
  if (WiFi_isConnected()) return true;
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
    system_soft_wdt_feed ();
    yield();
    delay(1000);
    system_soft_wdt_feed ();
    yield();
    snprintf_P(disp_buf, sizeof(disp_buf), PSTR("CON%02d"), i);
    disp(disp_buf);
  }
  snprintf_P(disp_buf, sizeof(disp_buf), PSTR(" %3.2f "), v);
  disp(disp_buf);
  return false;
}
