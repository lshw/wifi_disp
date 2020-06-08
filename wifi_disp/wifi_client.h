#ifndef __WIFI_CLIENT_H__
#define __WIFI_CLIENT_H__
#include "config.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
#include "ds1820.h"
extern bool power_in;
bool wifi_connected = false;
#if DHT_HAVE
extern float shidu, wendu;
bool dht_loop(); //不阻塞
void dht_load(); //阻塞等转换完成
#endif
void AP();
bool http_update();
void poweroff(uint32_t);
void ht16c21_cmd(uint8_t cmd, uint8_t dat);
ESP8266WiFiMulti WiFiMulti;
HTTPClient http;
String ssid, passwd;
uint8_t hex2ch(char dat) {
  dat |= 0x20; //41->61 A->a
  if (dat >= 'a') return dat - 'a' + 10;
  return dat - '0';
}
void wifi_setup() {
  File fp;
  uint32_t i;
  char buf[3];
  char ch;
  boolean is_ssid = true;
  if (proc == OTA_MODE) { //ota时要 ap 和 client
    WiFi.mode(WIFI_AP_STA);
    AP();
  } else  { //测温时， 只用client
    WiFi.mode(WIFI_STA);
  }
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  if (SPIFFS.begin()) {
    if (!SPIFFS.exists("/ssid.txt")) {
      fp = SPIFFS.open("/ssid.txt", "w");
      fp.println("test:cfido.com");
      fp.close();
    }
    fp = SPIFFS.open("/ssid.txt", "r");
    Serial.print("载入wifi设置文件:/ssid.txt ");
    ssid = "";
    passwd = "";
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
              ssid += ch;
            else
              passwd += ch;
        }
      }
      if (ssid != "" && passwd != "") {
        Serial.print("Ssid:"); Serial.println(ssid);
        Serial.print("Passwd:"); Serial.println(passwd);
        WiFiMulti.addAP(ssid.c_str(), passwd.c_str());
      }
    }
    fp.close();
    SPIFFS.end();
  }
}

bool wifi_connected_is_ok() {
  if (WiFiMulti.run() == WL_CONNECTED)
  {
    ht16c21_cmd(0x88, 0); //停止闪烁
    return true;
  }
  ht16c21_cmd(0x88, 0); //开始闪烁
  return false;
}

uint16_t http_get(uint8_t no) {
  char key[17];
  String url0 = get_url(no);
  if (url0.indexOf('?') > 0)
    url0 += '&';
  else
    url0 += '?';
  url0 += "ver="  VER  "&sn=" + hostname
          + "&ssid=" + String(WiFi.SSID())
          + "&bssid=" + WiFi.BSSIDstr()
          + "&batt=" + String(v)
          + "&rssi=" + String(WiFi.RSSI())
          + "&power=" + String(power_in)
          + "&change=" + String(nvram.nvram7 & NVRAM7_CHARGE)
          + "&temp=" + String(temp[0]);
#if DHT_HAVE
  dht_load();
  if (wendu > -300.0 && shidu >= 0.0 && shidu <= 100.0)
    url0 += "&shidu=" + String((int8_t)shidu) + "%," + String(wendu);
#endif
  if (dsn[1][0] != 0) {
    url0 += "&temps=";
    for (uint8_t i = 0; i < 32; i++) {
      if (dsn[i][0] == 0) continue;
      if (i > 0)
        url0 += ",";
      sprintf(key, "%02x%02x%02x:", dsn[i][0], dsn[i][6], dsn[i][7]);
      url0 += key + String(temp[i]);
    }
  }

  Serial.println( url0); //串口输出
  http.begin( url0 ); //HTTP提交
  http.setTimeout(4000);
  int httpCode;
  for (uint8_t i = 0; i < 10; i++) {
    httpCode = http.GET();
    if (httpCode < 0) {
      Serial.write('E');
      delay(20);
      continue;
    }
    // httpCode will be negative on error
    if (httpCode >= 200 && httpCode <= 299) {
      // HTTP header has been send and Server response header has been handled
      if (nvram.proc != 0) {
        nvram.proc = 0;
        nvram.change = 1;
      }
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
          SPIFFS.begin();
          if (http_update() == false)
            http_update();
          poweroff(1800);
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

      if (httpCode > 0)
        sprintf(disp_buf, ".E %03d", httpCode);
      disp(disp_buf);
      Serial.print("http error code ");
      Serial.println(httpCode);
      break;
    }
  }
  //  http.end();
  url0 = "";
  return httpCode;
}

void update_progress(int cur, int total) {
  char disp_buf[6];
  Serial.printf("HTTP update process at %d of %d bytes...\r\n", cur, total);
  sprintf(disp_buf, "HUP.%02d", cur * 99 / total);
  disp(disp_buf);
  ht16c21_cmd(0x88, 1); //闪烁
}

bool http_update()
{
  if (get_batt() < 3.6) {
    Serial.println("电压太低,不做升级");
    ESP.restart();
    return false;
  }
  if (nvram.nvram7 & NVRAM7_CHARGE == 0) {
    nvram.nvram7 |= NVRAM7_CHARGE; //开充电模式
    nvram.change = 1;
    save_nvram();
  }
  disp("H UP. ");
  String update_url = "http://www.anheng.com.cn/wifi_disp.bin";
  Serial.print("下载firmware from ");
  Serial.println(update_url);
  ESPhttpUpdate.onProgress(update_progress);
  t_httpUpdate_return  ret = ESPhttpUpdate.update(update_url);
  update_url = "";

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\r\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      if (nvram.proc != 0) {
        nvram.proc = 0;
        nvram.change = 1;
        save_nvram();
      }
      ESP.restart();
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      return true;
      break;
  }
  delay(1000);
  return false;
}
#endif __WIFI_CLIENT_H__
