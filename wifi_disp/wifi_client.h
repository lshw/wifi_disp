#ifndef __WIFI_CLIENT_H__
#define __WIFI_CLIENT_H__
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

//#include <WiFiClientSecure.h>
#include <ESP8266WiFiMulti.h>
extern char ram_buf[10];
extern String url;
extern bool power_in;
bool http_update();
void AP();
void send_ram();
void set_ram_check();
void ht16c21_cmd(uint8_t cmd, uint8_t dat);
ESP8266WiFiMulti WiFiMulti;
HTTPClient http;
bool wifi_connect() {
  File fp;
  uint32_t i;
  WiFi.mode(WIFI_STA);
  delay(10);

  char ch;
  String ssid = "";
  String passwd = "";

  boolean is_ssid = true;
  if (SPIFFS.begin()) {
    fp = SPIFFS.open("/ssid.txt", "r");
    if (!fp) {
      AP();
      proc = AP_MODE;
      return true;
    }
    Serial.print("载入wifi设置文件:/ssid.txt ");
    if (fp) {
      uint16_t Fsize = fp.size();
      Serial.print(Fsize);
      Serial.println("字节");
      for (i = 0; i < Fsize; i++) {
        ch = fp.read();
        switch (ch) {
          case 0xd:
          case 0xa:
            if (ssid != "") {
              Serial.print("Ssid:"); Serial.println(ssid);
              Serial.print("Passwd:"); Serial.println(passwd);
              WiFiMulti.addAP(ssid.c_str(), passwd.c_str());
            }
            is_ssid = true;
            ssid = "";
            passwd = "";
            break;
          case ' ':
          case ':':
            is_ssid = false;
            break;
          default:
            if (is_ssid)
              ssid = ssid + String(ch);
            else
              passwd = passwd + String(ch);
        }
      }
      if (ssid != "" && passwd != "") {
        Serial.print("Ssid:"); Serial.println(ssid);
        Serial.print("Passwd:"); Serial.println(passwd);
        WiFiMulti.addAP(ssid.c_str(), passwd.c_str());
      }
    }
    fp.close();
  }
  Serial.println("正在连接wifi.");
  // ... Give ESP 10 seconds to connect to station.
  unsigned long startTime = millis();
  i = 0;
  while (WiFiMulti.run() != WL_CONNECTED && millis() - startTime < 40000)
  {
    Serial.write('.');
    //Serial.print(WiFi.status());
    delay(1000);
    if (temp == 85.00) { //85度时，是ds1820未完成测试
      i++;
      if (i == 2) {
        i = 0;
        temp = get_temp();
        if (temp < -300.0) {
          delay(100); //crc error;
          ds_init();
        }
      }
    }
  }
  Serial.println();
  ht16c21_cmd(0x88, 0); //停止闪烁
  if (WiFiMulti.run() == WL_CONNECTED)
  {
    ram_buf[0] = 0;
    send_ram();
    Serial.println("wifi已链接");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("PSK: ");
    Serial.println(WiFi.psk());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  }
  else
    return false;
}

uint16_t http_get() {
  if (temp < -300.0) {
    temp = get_temp();
    if (temp < -300.0) {
      delay(10);
      temp = get_temp();
      if (temp < -300.0) {
        digitalWrite(12, LOW);
        digitalWrite(14, LOW);
        delay(10);
        ds_init();
        delay(500);
        temp = get_temp();
      }
    }
  }
  if (temp == 85.00) {
    ds.reset();
    ds.select(dsn);
    ds.write(0x44, 1);
    delay(1000);
    get_temp();
  }

  digitalWrite(12, LOW);
  digitalWrite(14, LOW);
  String url0 = url + "?ver="  VER  "&sn=" + hostname
                + "&ssid=" + String(WiFi.SSID())
                + "&key=" + String(key)
                + "&batt=" + String(v)
                + "&rssi=" + String(WiFi.RSSI())
                + "&power=" + String(power_in)
                + "&temp=" + String(temp)
                + "&charge=" + String(ram_buf[7] & 1);

  Serial.println( url0); //串口输出
  http.begin( url0 ); //HTTP提交

  int httpCode;

  for (uint8_t i = 0; i < 10; i++) {
    httpCode = http.GET();
    if (httpCode < 0) {
      Serial.write('E');
      continue;
    }
    // httpCode will be negative on error
    if (httpCode >= 200 && httpCode <= 299) {
      // HTTP header has been send and Server response header has been handled
      Serial.print("[HTTP] GET... code:");
      Serial.println(httpCode);
      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        payload.toCharArray(disp_buf, 15); //.1.2.3.4.5,1800
        uint8_t    i1 = payload.indexOf(',');
        Serial.println(disp_buf);
        Serial.println();
        if (  disp_buf[0] == 'U'
              && disp_buf[1] == 'P'
              && disp_buf[2] == 'D'
              && disp_buf[3] == 'A'
              && disp_buf[4] == 'T'
              && disp_buf[5] == 'E') {
          if (http_update() == false)
            http_update();
          Serial.flush();
          ESP.restart();
        }
        next_disp = disp_buf[i1 + 1] & 0xf;
        if (disp_buf[i1 + 2] >= '0' && disp_buf[i1 + 2] <= '9') {
          next_disp = next_disp * 10 + (disp_buf[i1 + 2] & 0xf);
          if (disp_buf[i1 + 3] >= '0' && disp_buf[i1 + 3] <= '9') {
            next_disp = next_disp * 10 + (disp_buf[i1 + 3] & 0xf);
            if (disp_buf[i1 + 4] >= '0' && disp_buf[i1 + 4] <= '9') {
              next_disp = next_disp * 10 + (disp_buf[i1 + 4] & 0xf);
            }
          }
        }
        next_disp = next_disp * 10;
        disp_buf[i1] = 0;
        disp(disp_buf);
        break;
      }
    } else {

      ram_buf[9] |= 0x10; //x1
      send_ram();
      if (httpCode > 0)
        sprintf(disp_buf, ".E %03d", httpCode);
      disp(disp_buf);
      Serial.print("http error code ");
      Serial.println(httpCode);
      break;
    }
  }
  http.end();
  return httpCode;
}
#endif __WIFI_CLIENT_H__
