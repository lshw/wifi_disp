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
bool dht_loop(); //不阻塞
void dht_load(); //阻塞等转换完成
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
  if(dat <0x10) Serial.write('0');
  Serial.print(dat,HEX);
}
void onClientConnected(const WiFiEventSoftAPModeStationConnected& evt){
  ap_client_linked = true;
  Serial.begin(115200);
  Serial.print("\r\nclient linked:");
  for(uint8_t i = 0; i < 6; i++)
    hexprint(evt.mac[i]);
  Serial.println();
  Serial.flush();
  ht16c21_cmd(0x88, 0); //停止闪烁
}

WiFiEventHandler ConnectedHandler;

void AP() {
  WiFi.mode(WIFI_AP_STA); //开AP
  WiFi.softAP("disp", "");
  Serial.print("IP地址: ");
  Serial.println(WiFi.softAPIP());
  Serial.flush();
  ConnectedHandler = WiFi.onSoftAPModeStationConnected(&onClientConnected);
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", WiFi.softAPIP());
  Serial.println("泛域名dns服务器启动");
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  yield();
}

void wifi_setup() {
  File fp;
  uint32_t i;
  char buf[3];
  char ch;
  boolean is_ssid = true;
  WiFi.mode(WIFI_STA);
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
    if(ssid == "")
        WiFiMulti.addAP("test", "cfido.com");
    WiFiMulti.run();
    fp.close();
    SPIFFS.end();
  }
}
bool connected_is_ok = false;
bool wifi_connected_is_ok() {
  if(connected_is_ok)
    return connected_is_ok;
  if(proc == OTA_MODE && ap_client_linked  && millis() > 10000) return false;  //ota有wifi客户连上来，或者超过10秒没有连上上游AP， 就不再尝试链接AP了
  if (WiFiMulti.run() == WL_CONNECTED)
  {
    connected_is_ok = true;
    ht16c21_cmd(0x88, 0); //停止闪烁
    if(nvram.ch != wifi_get_channel() ){
      nvram.ch =  wifi_get_channel();
      nvram.change = 1;
    }

    uint8_t ap_id=wifi_station_get_current_ap_id();
    struct station_config config[5];
    wifi_station_get_ap_info(config);
    config[ap_id].bssid_set = 1; //同名ap，mac地址不同
    wifi_station_set_config(&config[ap_id]); //保存成功的ssid,用于下次通讯

    return true;
  }
  if(proc != OTA_MODE)
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
  url0 += "ver="  VER  "&sn=" + hostname
          + "&ssid=" + String(WiFi.SSID())
          + "&bssid=" + WiFi.BSSIDstr()
          + "&batt=" + String(v)
          + "&rssi=" + String(WiFi.RSSI())
          + "&power=" + String(power_in)
          + "&change=" + String(nvram.nvram7 & NVRAM7_CHARGE)
          + "&ms=" + String(millis())
          + "&temp=" + String(temp[0]);
  if(nvram.have_dht >= 0){
    dht_load();
    if (wendu > -300.0 && shidu >= 0.0 && shidu <= 100.0)
      url0 += "&shidu=" + String((int8_t)shidu) + "%," + String(wendu);
  }
  if (dsn[1][0] != 0) {
    url0 += "&temps=";
    for (uint8_t i = 0; i < 32; i++) {
      if (dsn[i][0] == 0) continue;
      if (i > 0)
        url0 += ",";
      snprintf(key,sizeof(key), "%02x%02x%02x:", dsn[i][0], dsn[i][6], dsn[i][7]);
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
        snprintf(disp_buf,sizeof(disp_buf), ".E%4d", httpCode);
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
  snprintf(disp_buf,sizeof(disp_buf),"HUP.%2d", cur * 99 / total);
  ht16c21_cmd(0x88, 0); //停闪烁
  disp(disp_buf);
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
  String update_url = "http://www.anheng.com.cn/wifi_disp_new.bin";
  Serial.print("下载firmware from ");
  Serial.println(update_url);
  ESPhttpUpdate.onProgress(update_progress);
  t_httpUpdate_return  ret = ESPhttpUpdate.update(client, update_url);
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
