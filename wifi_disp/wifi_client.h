#ifndef __WIFI_CLIENT_H__
#define __WIFI_CLIENT_H__
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
//#include <Pinger.h>
//#include <WiFiClientSecure.h>
#include <ESP8266WiFiMulti.h>
#include "global.h"
//Pinger pinger;
extern char ram_buf[10];
extern bool power_in;
bool http_update();
void poweroff(uint32_t);
void AP();
void send_ram();
void set_ram_check();
void ht16c21_cmd(uint8_t cmd, uint8_t dat);
ESP8266WiFiMulti WiFiMulti;
HTTPClient http;
String ssid, passwd, bssidstr;
uint32_t channel = 0;
IPAddress local_ip, gateway, netmask, dns1, dns2;

void save_wifi() {
  File fp;
  if (ssid == WiFi.SSID()
      && passwd == WiFi.psk()
      && channel == WiFi.channel()
      && bssidstr == WiFi.BSSIDstr()
      && local_ip == WiFi.localIP()
      && gateway == WiFi.gatewayIP()
      && netmask == WiFi.subnetMask()
      && dns1 == WiFi.dnsIP(0)
      && dns2 == WiFi.dnsIP(1) ) return;

  if (!SPIFFS.begin()) return;
  fp = SPIFFS.open("/wifi_set.txt", "w");
  fp.println(WiFi.SSID());
  fp.println(WiFi.psk());
  fp.println(WiFi.channel());
  fp.println(WiFi.BSSIDstr());
  fp.println(WiFi.localIP().toString());
  fp.println(WiFi.gatewayIP().toString());
  fp.println(WiFi.subnetMask().toString());
  fp.println(WiFi.dnsIP(0).toString());
  fp.println(WiFi.dnsIP(1).toString());
  fp.close();
  Serial.println("保存wifi参数到 /wifi_set.txt");
}
uint8_t hex2ch(char dat) {
  dat |= 0x20; //41->61 A->a
  if (dat >= 'a') return dat - 'a' + 10;
  return dat - '0';
}
bool aping;
bool wifi_connect() {
  File fp;
  uint32_t i;
  uint8_t bssid[7];
  char buf[3];
  WiFi.mode(WIFI_STA);
  delay(10);

  char ch;
  boolean is_ssid = true;
  if (SPIFFS.begin()) {
   if(proc==OTA_MODE)
      SPIFFS.remove("/wifi_set.txt");
    fp = SPIFFS.open("/wifi_set.txt", "r");
    if (fp) {
      ssid = fp_gets(fp); //第一行ssid
      passwd = fp_gets(fp); //第二行 passwd
      channel = fp_gets(fp).toInt(); //第三行 频道
      bssidstr = fp_gets(fp); //第四行bssid
      bssid[0] = (hex2ch(bssidstr.charAt(0)) << 4) | hex2ch(bssidstr.charAt(1));
      bssid[1] = (hex2ch(bssidstr.charAt(3)) << 4) | hex2ch(bssidstr.charAt(4));
      bssid[2] = (hex2ch(bssidstr.charAt(6)) << 4) | hex2ch(bssidstr.charAt(7));
      bssid[3] = (hex2ch(bssidstr.charAt(9)) << 4) | hex2ch(bssidstr.charAt(10));
      bssid[4] = (hex2ch(bssidstr.charAt(12)) << 4) | hex2ch(bssidstr.charAt(13));
      bssid[5] = (hex2ch(bssidstr.charAt(15)) << 4) | hex2ch(bssidstr.charAt(16));
      bssid[6] = 0;
      local_ip.fromString(fp_gets(fp));//第五行ip
      gateway.fromString(fp_gets(fp));//第六行网关
      netmask.fromString(fp_gets(fp));//第七行掩码
      dns1.fromString(fp_gets(fp));//第八行dns1
      dns2.fromString(fp_gets(fp));//第九行dns2
      fp.close();
      WiFi.config(local_ip, gateway, netmask, dns1, dns2);
      WiFi.begin(ssid.c_str(), passwd.c_str(), channel, bssid);
      Serial.print("ssid=");
      Serial.println(ssid);
      Serial.print("passwd=");
      Serial.println(passwd);
      Serial.print("channel=");
      Serial.println(channel);
      Serial.print("bssid=");
      Serial.println(bssidstr);
      Serial.print("ip=");
      Serial.println(local_ip);
      Serial.print("netmask=");
      Serial.println(netmask);
      Serial.print("gateway=");
      Serial.println(gateway);
      Serial.print("dns1=");
      Serial.println(dns1);
      Serial.print("dns2=");
      Serial.println(dns2);
   /*
   aping = false;
      pinger.OnReceive([](const PingerResponse & response)
      {
        if (response.ReceivedResponse) {
          Serial.println("ping ok");
          aping = true;
          return false;
        } else
          Serial.println("ping timeout");
        // Return true to continue the ping sequence.
        // If current event returns false, the ping sequence is interrupted.
        return true;
      });

      pinger.Ping(gateway, 2, 1000);
      for (ch = 0; ch < 20; ch++) {
        delay(100);
        if (aping) {
          Serial.println("使用 /wifi_set.txt 上次通讯的设置联机成功");
          return true;
        }
        Serial.print('.');
      }

*/
    }
  }
  Serial.print("ping gateway=");
  Serial.println(aping);
  if (SPIFFS.begin()) {
    fp = SPIFFS.open("/ssid.txt", "r");
    if (!fp) {
      AP();
      proc = AP_MODE;
      return true;
    }
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
    if (i % 2 == 0) {
      if (temp_ok == false) temp_ok = get_temp();
    } else
      i++;
  }
  Serial.println();
  if (temp_ok == false) {
    if (millis() < (temp_start + 2000)) delay(temp_start + 2000 - millis());
    temp_ok = get_temp();
  }
  ht16c21_cmd(0x88, 0); //停止闪烁
  if (WiFiMulti.run() == WL_CONNECTED)
  {
    Serial.println("wifi已链接");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.println("BSSID: " + WiFi.BSSIDstr());
    Serial.print("PSK: ");
    Serial.println(WiFi.psk());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  }
  else
    return false;
}

uint16_t http_get(uint8_t no) {
  char key[17];
  String url0 = get_url(no) + "?ver="  VER  "&sn=" + hostname
                + "&ssid=" + String(WiFi.SSID())
                + "&bssid=" + WiFi.BSSIDstr()
                + "&batt=" + String(v)
                + "&rssi=" + String(WiFi.RSSI())
                + "&power=" + String(power_in)
                + "&charge=" + String(ram_buf[7] & 1)
                + "&temp=" + String(temp[0]);
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
      ram_buf[0] = 0;
      if (no == 0) ram_buf[7] &= ~2;
      else ram_buf[7] |= 2; //bit2表示上次完成通讯用的是哪个url 0:url0 2:url1
      send_ram();
      save_wifi();
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
          SPIFFS.remove("/wifi_set.txt");
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
