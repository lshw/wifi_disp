#ifndef __OTA_H__
#define __OTA_H__
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "global.h"
#include "httpd.h"
uint8_t ip_offset, ip_len;
extern void disp(char *);
extern float get_batt();
extern uint32_t ap_on_time;
extern bool power_in;
extern void poweroff(uint32_t sec);
void ota_setup() {
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    if (nvram.proc != 0) {
      nvram.proc = 0;
      nvram.change = 1;
    }
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
    type = "";
  });
  ArduinoOTA.onEnd([]() {
    ht16c21_cmd(0x88, 1); //闪烁
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    snprintf(disp_buf, sizeof(disp_buf), "OTA.%2d", progress * 99 / total );
    disp(disp_buf);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  Serial.println("OTA Ready");
}

#endif //__OTA_H__
