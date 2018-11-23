#ifndef __OTA_H__
#define __OTA_H__
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
char ip_buf[30],ip_offset,ip_len;
extern void disp(char *);
void ota_setup() {

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
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
  snprintf(ip_buf,sizeof(ip_buf),"OTP %s ",WiFi.localIP().toString().c_str());
  ip_len = strlen(ip_buf);
  ip_offset = 0;
  Serial.println("Ready");
  Serial.println(ip_len);
}
uint16_t sec0,sec1;
void ota_loop() {
  uint8_t i,i0,i1;
  if (millis() < 600000) {
    sec0=millis()/1000;
    if(sec0!=sec1){
      i=ip_offset;
      i0=0;
      i1=0;
      while(i1<5) {
	i=i%ip_len;
	disp_buf[i0]=ip_buf[i];
        disp_buf[i0+1]=0;
        i++;
        i0++;
	if(ip_buf[i]!='.') i1++;
        else if(i1==1)  { //第一个数字，第2个数字，都不能带小数点
        disp_buf[i0-1]=' ';
        disp_buf[i0]=' ';
}
      }
      ip_offset++;
      ip_offset%=ip_len;
    sec1=sec0;
    //最后一个点要消掉
    i0=strlen(disp_buf);
    if(disp_buf[i0-1]=='.')disp_buf[i0-1]=0;
    disp(disp_buf);
    }
    ArduinoOTA.handle();
  } else
    ESP.restart();
  return;
}

#endif //__OTA_H__
