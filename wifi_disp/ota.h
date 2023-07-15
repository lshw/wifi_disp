#ifndef __OTA_H__
#define __OTA_H__
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "global.h"
#include "httpd.h"
extern void disp(char *);
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
    Serial.print(F("Start updating "));
    Serial.println(type);
    upgrading = true;
    type = "";
  });
  ArduinoOTA.onEnd([]() {
    ht16c21_cmd(0x88, 1); //闪烁
    Serial.println(F("\nEnd"));
    upgrading = false;
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf_P(PSTR("Progress: %u%%\r"), (progress / (total / 100)));
    snprintf_P(disp_buf, sizeof(disp_buf), PSTR("OTA.%2d"), progress * 99 / total );
    ht16c21_cmd(0x88, 0); //停闪烁
    disp(disp_buf);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf_P(PSTR("Error[%u]: "), error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println(F("Auth Failed"));
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println(F("Begin Failed"));
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println(F("Connect Failed"));
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println(F("Receive Failed"));
    } else if (error == OTA_END_ERROR) {
      Serial.println(F("End Failed"));
    }
  });
  ArduinoOTA.begin();
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  Serial.println(F("OTA Ready"));
}

#endif //__OTA_H__
