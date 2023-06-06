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
extern float shidu, wendu;
void AP();
bool http_update();
void poweroff(uint32_t);
void ht16c21_cmd(uint8_t cmd, uint8_t dat);
ESP8266WiFiMulti WiFiMulti;
WiFiClient client;
HTTPClient http;
String ssid, passwd;
bool ap_client_linked = false;
uint8_t hex2ch(char dat) {
  dat |= 0x20; //41->61 A->a
  if (dat >= 'a') return dat - 'a' + 10;
  return dat - '0';
}
void hexprint(uint8_t dat) {
  if (dat < 0x10) Serial.write('0');
  Serial.print(dat, HEX);
}
void onClientConnected(const WiFiEventSoftAPModeStationConnected& evt) {
  ap_client_linked = true;
  Serial.begin(115200);
  Serial.print(F("\r\nclient linked:"));
  for (uint8_t i = 0; i < 6; i++)
    hexprint(evt.mac[i]);
  Serial.println();
  Serial.flush();
  ht16c21_cmd(0x88, 0); //停止闪烁
  if (power_in)
    ap_on_time = millis() + 1000000; //插AP模式1000秒
  else
    ap_on_time = millis() + 200000; //不插电AP模式200秒
}

WiFiEventHandler ConnectedHandler;

void AP() {
  WiFi.mode(WIFI_AP_STA); //开AP
  WiFi.softAP("disp", "");
  Serial.print(F("IP地址: "));
  Serial.println(WiFi.softAPIP());
  Serial.flush();
  ConnectedHandler = WiFi.onSoftAPModeStationConnected(&onClientConnected);
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", WiFi.softAPIP());
  Serial.println(F("泛域名dns服务器启动"));
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  yield();
}

void wifi_setup() {
  File fp;
  uint32_t i;
  char buf[3];
  char ch;
  uint8_t count = 0;
  boolean is_ssid = true;
  WiFi.mode(WIFI_STA);
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  if (SPIFFS.begin()) {
    if (!SPIFFS.exists("/ssid.txt")) {
      fp = SPIFFS.open("/ssid.txt", "w");
      fp.println(F("test:cfido.com"));
      fp.close();
    }
    fp = SPIFFS.open("/ssid.txt", "r");
    Serial.print(F("载入wifi设置文件:/ssid.txt "));
    ssid = "";
    passwd = "";
    if (fp) {
      uint16_t Fsize = fp.size();
      Serial.print(Fsize);
      Serial.println(F("字节"));
      for (i = 0; i < Fsize; i++) {
        ch = fp.read();
        switch (ch) {
          case 0xd:
          case 0xa:
            if (ssid != "") {
              Serial.print(F("Ssid:")); Serial.println(ssid);
              Serial.print(F("Passwd:")); Serial.println(passwd);
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
        if (count < 5) count ++;
        Serial.print(F("Ssid:")); Serial.println(ssid);
        Serial.print(F("Passwd:")); Serial.println(passwd);
        WiFiMulti.addAP(ssid.c_str(), passwd.c_str());
      }
    }
    if (count == 0)
      WiFiMulti.addAP("test", "cfido.com");
    fp.close();
    SPIFFS.end();
  }
  WiFi.setAutoConnect(true);//自动链接上次
  WiFi.setAutoReconnect(true);//断线自动重连
  WiFiMulti.run();
  wifi_connected_is_ok();
}
bool connected_is_ok = false;
bool wifi_connected_is_ok() {
  if (connected_is_ok)
    return connected_is_ok;
  if (proc == OTA_MODE && ap_client_linked  && millis() > 10000) return false; //ota有wifi客户连上来，或者超过10秒没有连上上游AP， 就不再尝试链接AP了
  if (wifi_station_get_connect_status() == STATION_GOT_IP) {
    connected_is_ok = true;
    ht16c21_cmd(0x88, 0); //停止闪烁
    if (nvram.ch != wifi_get_channel() ) {
      nvram.ch =  wifi_get_channel();
      nvram.change = 1;
    }

    uint8_t ap_id = wifi_station_get_current_ap_id();
    struct station_config config[5];
    wifi_station_get_ap_info(config);
    config[ap_id].bssid_set = 1; //同名ap，mac地址不同
    wifi_station_set_config(&config[ap_id]); //保存成功的ssid,用于下次通讯

    return true;
  }
  if (proc != OTA_MODE)
    ht16c21_cmd(0x88, 1); //开始闪烁
  return false;
}

uint16_t http_get(uint8_t no) {
  char key[17];
  String url0 = get_url(no);
  if (url0.indexOf('?') > 0)
    url0 += '&';
  else
    url0 += '?';
  url0 +=  "GIT=" GIT_VER "&ver=" VER "&sn=" + hostname
           + "&ssid=" + String(WiFi.SSID())
           + "&bssid=" + WiFi.BSSIDstr()
           + "&batt=" + String(v)
           + "&rssi=" + String(WiFi.RSSI())
           + "&power=" + String(power_in)
           + "&charge=" + String(nvram.nvram7 & NVRAM7_CHARGE)
           + "&ms=" + String(millis())
           + "&temp=" + String(temp[0]);
  if (wendu > -300.0 && shidu >= 0.0 && shidu <= 100.0)
    url0 += "&shidu=" + String((int8_t)shidu) + "%," + String(wendu);
  if (dsn[1][0] != 0) {
    url0 += "&temps=";
    for (uint8_t i = 0; i < 32; i++) {
      if (dsn[i][0] == 0) continue;
      if (i > 0)
        url0 += ",";
      snprintf_P(key, sizeof(key), PSTR("%02x%02x%02x:"), dsn[i][0], dsn[i][6], dsn[i][7]);
      url0 += key + String(temp[i]);
    }
  }

  Serial.println( url0); //串口输出
  http.begin(client, url0 ); //HTTP提交
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
      Serial.printf_P(PSTR("[HTTP] GET... code:%d\r\n"), httpCode);
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
          ht16c21_cmd(0x88, 0); //停闪烁
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
        snprintf_P(disp_buf, sizeof(disp_buf), PSTR(".E%4d"), httpCode);
      disp(disp_buf);
      Serial.print(F("http error code "));
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
  Serial.printf_P(PSTR("HTTP update process at %d of %d bytes...\r\n"), cur, total);
  snprintf_P(disp_buf, sizeof(disp_buf), PSTR("HUP%2d"), cur * 99 / total);
  ht16c21_cmd(0x88, 0); //停闪烁
  disp(disp_buf);
}

bool http_update()
{
  if (get_batt() < 3.6) {
    Serial.println(F("电压太低,不做升级"));
    ESP.restart();
    return false;
  }
  if (nvram.nvram7 & NVRAM7_CHARGE == 0) {
    nvram.nvram7 |= NVRAM7_CHARGE; //开充电模式
    nvram.change = 1;
    save_nvram();
  }
  disp((char *)"H UP. ");
  String update_url = "http://wifi_disp.anheng.com.cn/firmware.php?type=WIFI_DISP&SN=" + hostname + "&GIT=" GIT_VER "&ver=" VER;
  Serial.print(F("下载firmware from "));
  Serial.println(update_url);
  ESPhttpUpdate.onProgress(update_progress);
  t_httpUpdate_return  ret = ESPhttpUpdate.update(client, update_url);
  update_url = "";

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf_P(PSTR("HTTP_UPDATE_FAILD Error (%d): %s\r\n"), ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      if (nvram.proc != 0) {
        nvram.proc = 0;
        nvram.change = 1;
        save_nvram();
      }
      ESP.restart();
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println(F("HTTP_UPDATE_NO_UPDATES"));
      break;

    case HTTP_UPDATE_OK:
      Serial.println(F("HTTP_UPDATE_OK"));
      return true;
      break;
  }
  delay(1000);
  return false;
}
#endif __WIFI_CLIENT_H__
