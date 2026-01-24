#ifndef __WIFI_CLIENT_H__
#define __WIFI_CLIENT_H__
#include "config.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
#include "ds1820.h"
extern bool power_in;
extern float shidu, wendu;
extern uint8_t rxBuf[250], rxLen;
void AP();
bool http_update(String update_url);
void poweroff(uint32_t);
void web_cmd(String str);
void web_cmd_a(String str);
void ht16c21_cmd(uint8_t cmd, uint8_t dat);
ESP8266WiFiMulti WiFiMulti;
WiFiClient client;
String ssid, passwd;
bool ap_client_linked = false;
uint8_t hex2ch(char dat) {
  dat |= 0x20;  //41->61 A->a
  if (dat >= 'a') return dat - 'a' + 10;
  return dat - '0';
}
void hexprint(uint8_t dat) {
  if (dat < 0x10) Serial.write('0');
  Serial.print(dat, HEX);
}
void onClientConnected(const WiFiEventSoftAPModeStationConnected& evt) {
  add_limit_millis();
  ap_client_linked = true;
  Serial_begin();
  Serial.print(F("\r\nclient linked:"));
  for (uint8_t i = 0; i < 6; i++)
    hexprint(evt.mac[i]);
  Serial.println();
  Serial.flush();
  ht16c21_cmd(0x88, 0);  //停止闪烁
}

WiFiEventHandler ConnectedHandler;

void AP() {
  WiFi.mode(WIFI_AP_STA);  //开AP
  WiFi.softAP("disp", "");
  Serial.print(F("IP地址: "));
  Serial.println(WiFi.softAPIP());
  ConnectedHandler = WiFi.onSoftAPModeStationConnected(&onClientConnected);
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", WiFi.softAPIP());
  Serial.println(F("泛域名dns服务器启动"));
  Serial.flush();
  yield();
}

void wifi_setup() {
  File fp;
  uint32_t i;
  char buf[3];
  char ch;
  uint8_t count = 0;
  boolean is_ssid = true;
  /*
    wifi_country_t mycountry =
    {
     .cc = "CN",
     .schan = 1,
     .nchan = 14, //13
     .policy = WIFI_COUNTRY_POLICY_MANUAL,
    };
    wifi_set_country(&mycountry);
  */
  if (proc == SETUP_MODE)
    AP();
  else
    WiFi.mode(WIFI_STA);
  dump_ap_config();
  WiFi.setAutoConnect(true);  //自动链接上次
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
      Serial.println(F("字节\r\n_____"));
      for (i = 0; i < Fsize; i++) {
        ch = fp.read();
        switch (ch) {
          case 0xd:
          case 0xa:
            if (ssid != "") {
              Serial.print(F("Ssid:"));
              Serial.println(ssid);
              Serial.print(F("Passwd:"));
              Serial.println(passwd);
              WiFiMulti.addAP(ssid.c_str(), passwd.c_str());
              Serial.println(F("-----"));
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
        if (count < 5) count++;
        Serial.print(F("Ssid:"));
        Serial.println(ssid);
        Serial.print(F("Passwd:"));
        Serial.println(passwd);
        WiFiMulti.addAP(ssid.c_str(), passwd.c_str());
        Serial.println(F("-----"));
      }
    }
    if (count == 0)
      WiFiMulti.addAP("test", "cfido.com");
    fp.close();
    SPIFFS.end();
  }
  WiFiMulti.run(10000);
}
bool connected_is_ok = false;
bool fast_wifi = true;
bool WiFi_isConnected() {
  if (connected_is_ok)
    return connected_is_ok;
  if (proc == SETUP_MODE && ap_client_linked) return false;  //ota有wifi客户连上来,就不再尝试链接AP了
  if (fast_wifi && millis() > 6000) {
    fast_wifi = false;  //5秒钟没有登陆， 就要用常规登陆了
    wifi_setup();
  }
  if (fast_wifi) {
    if (WiFi.status() == WL_CONNECTED && WiFi.localIP()) {
      dump_ap_config();
      connected_is_ok = true;
      Serial.printf_P(PSTR("\r\n用上次的信息登陆ap成功,millis()=%ld\r\n"), millis());
    }
  } else if (WiFi.localIP()) {
    uint8_t ap_id = wifi_station_get_current_ap_id();
    struct station_config config[5];
    wifi_station_get_ap_info(config);
    config[ap_id].bssid_set = 1;  //同名ap，mac地址不同
    config[ap_id].channel = wifi_get_channel();
    wifi_station_set_config(&config[ap_id]);  //保存成功的ssid,用于下次通讯
    connected_is_ok = true;
    Serial.printf_P(PSTR("\r\n用SSID设置登陆ap成功,ch=%d,millis()=%ld\r\n"), config[ap_id].channel, millis());
  }
  if (connected_is_ok == true) {
    Serial.println(WiFi.localIP());
    ht16c21_cmd(0x88, 0);  //停止闪烁
    return true;
  }
  if (proc != SETUP_MODE)
    ht16c21_cmd(0x88, 1);  //开始闪烁
  return false;
}

int16_t get_content_length(uint16_t& content_length) {
  int16_t httpCode;
  uint32_t timeoutMs = millis() + 10000;
  while (millis() < timeoutMs) {
    if (!client.available()) {
      delay(5);
      continue;
    }

    String line = client.readStringUntil('\n');
    line.trim();
    Serial.println(String(millis()) + ":" + line);

    // 空行 = header 结束
    if (line.length() == 0) {
      return httpCode;
    }

    // Status line
    if (line.startsWith("HTTP/")) {
      // HTTP/1.0 200 OK
      httpCode = line.substring(9, 12).toInt();
    }
    // Content-Length
    else if (line.startsWith("Content-Length:")) {
      content_length = line.substring(15).toInt();
    }
  }
  return -11;
}

int16_t wget() {
  char key[17];
  String url0;
  bool no0, no = nvram.nvram7 & NVRAM7_URL;
  no0 = no;

  url0 += "GIT=" GIT_VER "&ver=" VER "&sn=" + hostname
          + "&ssid=" + String(WiFi.SSID())
          + "&bssid=" + WiFi.BSSIDstr()
          + "&batt=" + String(v)
          + "&rssi=" + String(WiFi.RSSI())
          + "&power=" + String(power_in)
          + "&pcb_ver=" + String(nvram.pcb_ver)
          + "&charge=" + String(nvram.nvram7 & NVRAM7_CHARGE)
          + "&ms=" + String(millis());
  if (proc == PROC4_MODE && rxLen > 0)
    url0 += "&lora=" + base64::encode(rxBuf, rxLen);
  if (bmp.begin()) {
    url0 += "&qiya=" + String(bmp.readPressure());
  }
  if (wendu < -299.0 || wendu == 85.00)
    get_temp();
  if (wendu > -300.0) {
    url0 += "&temp=" + String(wendu);
  }
  if (shidu >= 0.0 && shidu <= 100.0)
    url0 += "&shidu=" + String(shidu);
  if (bmp.begin()) {
    url0 += "&haiba=" + String(bmp.readAltitude())
            + "&wendu1=" + String(bmp.readTemperature())
            + "&qiya=" + String(bmp.readPressure());
  }
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

  int16_t httpCode = -1;
  String payload;
  struct ParsedURL u;
  IPAddress ip;
  for (uint8_t i = 0; i < 4; i++) {
    Serial.println(String(millis()) + "ms," + String(i) + "," + String(no) + ":" + get_url(no) + url0);  //串口输出
    parseURL(get_url(no), u);
    if (u.path == "?")
      u.path = "/?";
    if (no) ip = nvram.ip_addr[1];
    else ip = nvram.ip_addr[0];
    if (i > 1 || ip == IPAddress(0, 0, 0, 0)) {
      Serial.println(F("dns解析:") + u.host);
      if (WiFi.hostByName(u.host.c_str(), ip)) {
        Serial.println(F("dns解析ok") + String(no) + ip.toString());
        if (no) {
          if (nvram.ip_addr[1] != ip) {
            set0.dns_ip_change = 1;
            nvram.ip_addr[1] = ip;
          }
        } else {
          set0.dns_ip_change = 1;
          nvram.ip_addr[0] = ip;
        }
      }
    }
    Serial.println(F("连接:") + ip.toString() + ":" + String(u.port));
    if (!client.connect(ip, u.port)) {
      no = !no;
      continue;
    }
    client.print(F("GET ") + u.path + url0
                 + F(" HTTP/1.0\r\n"
                     "Host: ")
                 + u.host + "\r\n"
                 + F("User-Agent: wifi_disp\r\n"
                     "Connection: close\r\n\r\n"));
    uint16_t content_length = 0;
    delay(50);
    httpCode = get_content_length(content_length);
    Serial.println(F("httpCode=") + String(httpCode) + F(",content_length=") + String(content_length));
    uint32_t ms;
    char ch;
    ms = millis() + 1000;
    while (ms > millis()) {
      while (client.available()) {
        ch = client.read();
        payload += ch;
      }
      if (payload.length() >= content_length) break;
    }
    payload.trim();
    if (payload.length() == 0) {
      no = !no;  //换服务器
      snprintf_P(disp_buf, sizeof(disp_buf), PSTR(".E%4d"), httpCode);
      disp(disp_buf);
      Serial.print(F("http error code "));
      Serial.println(httpCode);
      delay(100);
      continue;
    }
    Serial.println(payload);
    // httpCode will be negative on error
    if (httpCode >= 200 && httpCode < 300) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf_P(PSTR("[HTTP] GET... code:%d\r\n"), httpCode);
      // file found at server
      char ch = payload.charAt(0);
      if ((ch < '0' || ch > '9') && ch != ',') {  //非数字， 就是web下发的命令
        web_cmd(payload);
        disp(F("8.8.8.8.8."));
        break;
      }

      memset(disp_buf, 0, sizeof(disp_buf));
      payload.toCharArray(disp_buf, sizeof(disp_buf) - 1);  //.1.2.3.4.5,1800
      uint8_t i1 = payload.indexOf(',');
      Serial.println(disp_buf);
      next_disp = atoi(&disp_buf[i1 + 1]);
      if (next_disp < 6)
        next_disp = 6;
      if (next_disp > 360 * 24)
        next_disp = 360 * 24;
      next_disp = next_disp * 10;
      disp_buf[i1] = 0;
      disp(disp_buf);
      if (httpCode >= 200 && no != no0) {
        nvram.nvram7 ^= (1 << NVRAM7_URL);  //先试试上次成功的url
        nvram.change = 1;
        save_nvram();
      }
      break;
    }
  }
  if (httpCode < 0) {
    snprintf_P(disp_buf, sizeof(disp_buf), PSTR(".E%4d"), httpCode);
    disp(disp_buf);
  }
  url0 = "";
  return httpCode;
}

void update_progress(int cur, int total) {
  char disp_buf[6];
  add_limit_millis();
  Serial.printf_P(PSTR("HTTP update process at %d of %d bytes...\r\n"), cur, total);
  snprintf_P(disp_buf, sizeof(disp_buf), PSTR("HUP%2d"), cur * 99 / total);
  ht16c21_cmd(0x88, 0);  //停闪烁
  disp(disp_buf);
}

bool http_update(String update_url) {
  if (get_batt() < 3.6) {
    Serial.println(F("电压太低,不做升级"));
    ESP.restart();
    return false;
  }
  if (nvram.nvram7 & NVRAM7_CHARGE == 0) {
    nvram.nvram7 |= NVRAM7_CHARGE;  //开充电模式
    nvram.change = 1;
    save_nvram();
  }
  upgrading = true;
  disp(F("H UP. "));
  if (update_url.length() == 0)
    update_url = "http://wifi_disp.anheng.com.cn/firmware.php?";
  else {
    if (update_url.indexOf('?') == -1)
      update_url += "?";
    else
      update_url += "&";
  }
  update_url += "type=WIFI_DISP&SN=" + hostname + "&GIT=" GIT_VER "&ver=" VER;
  Serial.print(F("下载firmware from "));
  Serial.println(update_url);
  ESPhttpUpdate.onProgress(update_progress);
  t_httpUpdate_return ret = ESPhttpUpdate.update(client, update_url);
  update_url = "";

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf_P(PSTR("HTTP_UPDATE_FAILD Error (%d): %s\r\n"), ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      if (nvram.proc != GENERAL_MODE) {
        nvram.proc = GENERAL_MODE;
        nvram.change = 1;
        save_nvram();
      }
      ESP.restart();
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println(F("HTTP_UPDATE_NO_UPDATES"));
      upgrading = false;
      break;

    case HTTP_UPDATE_OK:
      Serial.println(F("HTTP_UPDATE_OK"));
      upgrading = false;
      ht16c21_cmd(0x88, 1);  //0-不闪 1-2hz 2-1hz 3-0.5hz
      return true;
      break;
  }
  delay(1000);
  return false;
}
void web_cmd(String str) {  //处理下发的web命令
  String str0;
  int16_t i;
  str0 = str;
  uint8_t count = 0;
  while (str0 != "") {
    i = str0.indexOf(0xa);
    if (i < 0) i = str0.length();
    if (i == 0) break;
    web_cmd_a(str0.substring(0, i));
    count++;
    if (count > 5) break;
    if (i == str0.length()) break;
    str0 = str0.substring(i, str0.length());
  }
}
void web_cmd_a(String str) {
  String cmd, argv;
  str.trim();
  int16_t i = str.indexOf('=');
  int16_t len = str.length();
  if (i < 0) i = len;
  else argv = str.substring(i + 1, len);
  cmd = str.substring(0, i);
  Serial.print(F("cmd:["));
  Serial.print(cmd);
  Serial.println(']');
  if (cmd == "UPDATE") {
    ht16c21_cmd(0x88, 0);  //停闪烁
    if (http_update(argv) == false)
      http_update(argv);
    poweroff(1800);
  } else if (cmd == "WIFI_SET_ADD") {
    SPIFFS.begin();
    int16_t i0;
    i0 = str.indexOf(':');
    if (i0 > i) {
      wifi_set_add(str.substring(i + 1, i0).c_str(), str.substring(i0 + 1, len).c_str());
    }
  } else if (cmd == "OFF") {
    memset(disp_buf, ' ', sizeof(disp_buf));
    disp(disp_buf);
    poweroff(0);
  }
}
#endif __WIFI_CLIENT_H__
