#ifndef __AP_WEB_H__
#define __AP_WEB_H__
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include "wifi_client.h"
extern void disp(char *);
extern String hostname;
void poweroff(uint32_t);
float get_batt();
void ht16c21_cmd(uint8_t cmd, uint8_t dat);

DNSServer dnsServer;
ESP8266WebServer httpd(80);
void http204() {
  httpd.send(204, "", "");
  httpd.client().stop();
}
uint32_t ap_on_time = 120000;
void handleRoot() {
  String wifi_stat, wifi_scan;
  String ssid;
  int n = WiFi.scanNetworks();
  if (n > 0) {
    wifi_scan = "自动扫描到如下WiFi,点击连接:<br>";
    for (int i = 0; i < n; ++i) {
      ssid = String(WiFi.SSID(i));
      if (WiFi.encryptionType(i) != ENC_TYPE_NONE)
        wifi_scan += "&nbsp;<button onclick=get_passwd('" + ssid + "')>*";
      else
        wifi_scan += "&nbsp;<button onclick=select_ssid('" + ssid + "')>";
      wifi_scan += String(WiFi.SSID(i)) + "(" + String(WiFi.RSSI(i)) + "dbm)";
      wifi_scan += "</button>";
      delay(10);
    }
    wifi_scan += "<br>";
  }
  yield();
  if (WiFiMulti.run() == WL_CONNECTED) {
    wifi_stat = "wifi已连接 ssid:<mark>" + String(WiFi.SSID()) + "</mark> &nbsp; "
                + "ap:<mark>" + WiFi.BSSIDstr() + "</mark> &nbsp; "
                + "信号:<mark>" + String(WiFi.RSSI()) + "</mark>dbm &nbsp; "
                + "ip:<mark>" + WiFi.localIP().toString() + "</mark> &nbsp; "
                + "电池电压:<mark>" + String(v) + "</mark>V &nbsp; ";
    if(nvram.have_dht > 0) {
      dht_load();
      yield();
      if (wendu > -300.0 && shidu >= 0.0 && shidu <= 100.0)
        wifi_stat += "湿度:<mark>" + String((int8_t)shidu) + "%</mark> &nbsp; " + "温度:<mark>" + String(wendu) + "</mark>&#8451<br>";
      else
        wifi_stat += "温度:<mark>" + String(temp[0]) + "</mark>&#8451<br>";
    }else{
      wifi_stat += "温度:<mark>" + String(temp[0]) + "</mark>&#8451<br>";
   }
  }
  httpd.send(200, "text/html", "<html>"
             "<head>"
             "<meta http-equiv=Content-Type content='text/html;charset=utf-8'>"
             "<script>"
             "function get_passwd(ssid) {"
             "var passwd=prompt('输入 '+ssid+' 的密码:');"
             "if(passwd==null) return false;"
             "if(passwd) location.replace('add_ssid.php?data='+ssid+':'+passwd);"
             "else return false;"
             "return true;"
             "}"
             "function select_ssid(ssid){"
             "if(confirm('连接到['+ssid+']?')) location.replace('add_ssid.php?data='+ssid);"
             "}"
             "</script>"
             "</head>"
             "<body>"
             "SN:<mark>" + hostname + "</mark> &nbsp; "
             "版本:<mark>" VER "</mark>"
             "<hr>"
             + wifi_stat + "<hr>" + wifi_scan +
             "<hr><form action=/save.php method=post>"
             "输入ssid:passwd(可以多行多个)"
             "<input type=submit value=save><br>"
             "<textarea  style='width:500px;height:80px;' name=data>" + get_ssid() + "</textarea><br>"
             "可以设置自己的服务器地址(清空恢复)<br>"
             "url0:<input maxlength=100  size=30 type=text value='" + get_url(0) + "' name=url><br>"
             "url1:<input maxlength=100  size=30 type=text value='" + get_url(1) + "' name=url1><br>"
             "<input type=submit name=submit value=save>"
             "</form>"
             "<hr>"
             "<form method='POST' action='/update.php' enctype='multipart/form-data'>上传更新固件firmware:<input type='file' name='update'><input type='submit' value='Update'></form>"
             "<hr><table width=100%><tr><td align=left width=50%>程序源码:<a href=https://github.com/lshw/wifi_disp target=_blank>https://github.com/lshw/wifi_disp</a><td><td align=right width=50%>程序编译时间: <mark>" __DATE__ " " __TIME__ "</mark></td></tr></table>"
             "<hr></body>"
             "</html>");
  httpd.client().stop();
}
void handleNotFound() {
  File fp;
  int ch;
  String message;
  SPIFFS.begin();
  if (SPIFFS.exists(httpd.uri().c_str())) {
    fp = SPIFFS.open(httpd.uri().c_str(), "r");
    if (fp) {
      while (1) {
        ch = fp.read();
        if (ch == -1) break;
        message += (char)ch;
      }
      fp.close();
      httpd.send ( 200, "text/plain", message );
      httpd.client().stop();
      message = "";
      return;
    }
  }
  yield();
  message = "File Not Found\n\n";
  message += "URI: ";
  message += httpd.uri();
  httpd.send ( 404, "text/plain", message );
  httpd.client().stop();
  message = "";
}
void http_add_ssid() {
  File fp;
  String ssid, data;
  char ch;
  SPIFFS.begin();
  for (uint8_t i = 0; i < httpd.args(); i++) {
    if (httpd.argName(i).compareTo("data") == 0) {
      data = httpd.arg(i);
      data.trim();
      data.replace("\xef\xbc\x9a", ":"); //utf8 :
      data.replace("\xa3\xba", ":"); //gbk :
      data.replace("\xa1\x47", ":"); //big5 :
    }
  }
  if (data == "") return;
  fp = SPIFFS.open("/ssid.txt", "r");
  if (fp) {
    uint16_t Fsize = fp.size();
    ssid = "";
    for (uint16_t i = 0; i < Fsize; i++) {
      ch = fp.read();
      if (ch == 0xd || ch == 0xa ) {
        if (ssid != "" && ssid != data) { //忽略空行或者重复
          data += "\r\n" + ssid;
        }
        ssid = "";
        continue;
      } else ssid += ch;
    }
    fp.close();
  }
  SPIFFS.remove("/ssid.txt");
  fp = SPIFFS.open("/ssid.txt", "w");
  fp.println(data);
  fp.close();
  SPIFFS.end();
  httpd.send(200, "text/html", "<html><head></head><body><script>location.replace('/');</script></body></html>");
  yield();
}
void httpsave() {
  File fp;
  String url, data;
  SPIFFS.begin();
  for (uint8_t i = 0; i < httpd.args(); i++) {
    if (httpd.argName(i).compareTo("data") == 0) {
      data = httpd.arg(i);
      data.trim();
      data.replace("\xef\xbc\x9a", ":"); //utf8 :
      data.replace("\xa3\xba", ":"); //gbk :
      data.replace("\xa1\x47", ":"); //big5 :
      if (data.length() > 8) {
        Serial.println("data:[" + data + "]");
        //  Serial.print(data);
        // Serial.println("]");
        fp = SPIFFS.open("/ssid.txt", "w");
        fp.println(data);
        fp.close();
        fp = SPIFFS.open("/ssid.txt", "r");
        Serial.print("保存wifi设置到文件/ssid.txt ");
        Serial.print(fp.size());
        Serial.println("字节");
        fp.close();
      }
    } else if (httpd.argName(i).compareTo("url") == 0) {
      url = httpd.arg(i);
      url.trim();
      if (url.length() == 0) {
        Serial.println("删除url0设置");
        SPIFFS.remove("/url.txt");
      } else {
        Serial.print("url0:[");
        Serial.print(url);
        Serial.println("]");
        fp = SPIFFS.open("/url.txt", "w");
        fp.println(url);
        fp.close();
      }
    } else if (httpd.argName(i).compareTo("url1") == 0) {
      url = httpd.arg(i);
      url.trim();
      if (url.length() == 0) {
        Serial.println("删除url1设置");
        SPIFFS.remove("/url1.txt");
      } else {
        Serial.print("url1:[");
        Serial.print(url);
        Serial.println("]");
        fp = SPIFFS.open("/url1.txt", "w");
        fp.println(url);
        fp.close();
      }
    }
  }
  url = "";
  SPIFFS.end();
  httpd.send(200, "text/html", "<html><head></head><body><script>location.replace('/');</script></body></html>");
  yield();
}
void AP() {
  // Go into software AP mode.
  struct softap_config cfgESP;

  Serial.println("AP模式启动.\r\nssid:disp\r\npasswd:none");

  while (!wifi_softap_get_config(&cfgESP)) {
    system_soft_wdt_feed();
  }
  cfgESP.authmode = AUTH_OPEN;//无密码模式
  wifi_softap_set_config(&cfgESP);
  delay(10);
  WiFi.softAP("disp", "");
  Serial.print("IP地址: ");
  Serial.println(WiFi.softAPIP());
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", WiFi.softAPIP());
  Serial.println("泛域名dns服务器启动");
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  yield();
}
void httpd_listen() {

  httpd.begin();

  httpd.on("/", handleRoot);
  httpd.on("/save.php", httpsave); //保存设置
  httpd.on("/add_ssid.php", http_add_ssid); //保存设置
  httpd.on("/generate_204", http204);//安卓上网检测

  httpd.on("/update.php", HTTP_POST, []() {
    if (nvram.proc != 0) {
      nvram.proc = 0;
      nvram.change = 1;
      save_nvram();
    }
    httpd.sendHeader("Connection", "close");
    if (Update.hasError()) {
      Serial.println("上传失败");
      httpd.send(200, "text/html", "<html>"
                 "<head>"
                 "<meta http-equiv=Content-Type content='text/html;charset=utf-8'>"
                 "</head>"
                 "<body>"
                 "升级失败 <a href=/>返回</a>"
                 "</body>"
                 "</html>"
                );
    } else {
      httpd.send(200, "text/html", "<html>"
                 "<head>"
                 "<meta http-equiv=Content-Type content='text/html;charset=utf-8'>"
                 "</head>"
                 "<body>"
                 "<script>setTimeout(function(){ alert('升级成功!'); }, 15000); </script>"
                 "</body>"
                 "</html>"
                );
      Serial.println("上传成功");
      Serial.flush();
      ht16c21_cmd(0x88, 1); //闪烁
      delay(5);
      ESP.restart();
    }
    yield();
  }, []() {
    HTTPUpload& upload = httpd.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.setDebugOutput(true);
      WiFiUDP::stopAll();
      Serial.printf("Update: %s\r\n", upload.filename.c_str());
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      sprintf(disp_buf, "UP.%d", upload.totalSize / 1000);
      disp(disp_buf);
      Serial.println("size:" + String(upload.totalSize));
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\r\nRebooting...\r\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
      Serial.setDebugOutput(false);
    }
    yield();
  });
  httpd.onNotFound(handleNotFound);
  httpd.begin();

  Serial.printf("HTTP服务器启动! 用浏览器打开 http://%s.local\r\n", hostname.c_str());
}
#define httpd_loop() httpd.handleClient()

uint32_t ms0;
void ap_loop() {
  dnsServer.processNextRequest();
  httpd_loop();
  ArduinoOTA.handle();
  yield();
  if (ms0 < millis()) {
    get_batt();
    system_soft_wdt_feed ();
    Serial.print("batt:");
    Serial.print(get_batt());
    Serial.print("V,millis()=");
    Serial.println(ms0);
    ms0 = (ap_on_time - millis()) / 1000;
    if (ms0 < 10) sprintf(disp_buf, "AP  %d", ms0);
    else if (ms0 < 100) sprintf(disp_buf, "AP %d", ms0);
    else sprintf(disp_buf, "AP%d", ms0);
    ms0 = millis() + 1000;
    Serial.println(disp_buf);
    disp(disp_buf);

    if ( millis() > ap_on_time) {
      if (power_in && millis() < 1800000 ) ap_on_time = millis() + 200000; //有外接电源的情况下，最长半小时
      else {
        Serial.print("batt:");
        Serial.print(v);
        Serial.print("V,millis()=");
        Serial.println(millis());
        Serial.println("power down");
        if (nvram.proc != 0) {
          nvram.proc = 0;
          nvram.change = 1;
          save_nvram();
        }
        disp("00000");
        ht16c21_cmd(0x84, 0);
        httpd.close();
        poweroff(3600);
      }
    }
    yield();
    if (power_in == 1) {
      if (ds_pin != 0) digitalWrite(13, HIGH);
      else {
        Serial.flush();
        Serial.end();
        pinMode(1, OUTPUT);
        digitalWrite(1, HIGH);
      }
    }
  }
}
#endif //__AP_WEB_H__
