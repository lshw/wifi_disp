#ifndef __AP_WEB_H__
#define __AP_WEB_H__
#include <ESP8266WebServer.h>
#include <DNSServer.h>
extern void disp(char *);
extern char ram_buf[10];
extern String hostname;
void set_ram_check();
void send_ram();
void poweroff(uint32_t);
float get_batt();
void ht16c21_cmd(uint8_t cmd, uint8_t dat);

DNSServer dnsServer;
ESP8266WebServer server(80);
void http204() {
  server.send(204, "", "");
  server.client().stop();
}
uint32_t ap_on_time = 120000;
void handleRoot() {
  server.send(200, "text/html", "<html>"
              "<head>"
              "<meta http-equiv=Content-Type content='text/html;charset=utf-8'>"
              "</head>"
              "<body>"
              "SN:"+hostname+"<br>"
              "<form action=/save.php method=post>"
              "输入ssid:passwd(可以多行多个)"
              "<input type=submit value=save><br>"
              "<textarea  style='width:500px;height:80px;' name=data>" + get_ssid() + "</textarea><br>"
              "url:<input maxlength=100  size=50 type=text value='" + get_url() + "' name=url>(如果不知道此设置的作用，请不要修改)<br>"
              "<input type=submit name=submit value=save>"
              "</form>"
              "<hr>"
              "</body>"
              "</html>");
  server.client().stop();
  ap_on_time = millis() + 200000;
}
void httpsave() {
  File fp;
  SPIFFS.begin();
  for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i).compareTo("data") == 0) {
      String data = server.arg(i);
      data.trim();
      if (data.length() > 8) {
        Serial.println("data:[" + data + "]");
        //  Serial.print(data);
        // Serial.println("]");
        fp = SPIFFS.open("/ssid.txt", "w");
        fp.println(server.arg(i));
        fp.close();
        fp = SPIFFS.open("/ssid.txt", "r");
        Serial.print("保存wifi设置到文件/ssid.txt ");
        Serial.print(fp.size());
        Serial.println("字节");
        fp.close();
      }
    }
    if (server.argName(i).compareTo("url") == 0) {
      url = server.arg(i);
      url.trim();
      if (url.length() == 0) {
        Serial.println("删除url设置");
        SPIFFS.remove("/url.txt");
      } else {
        Serial.print("url:[");
        Serial.print(url);
        Serial.println("]");
        fp = SPIFFS.open("/url.txt", "w");
        fp.println(url);
        fp = SPIFFS.open("/url.txt", "r");
        Serial.print("保存url设置到文件/url.txt ");
        Serial.print(fp.size());
        Serial.println("字节");
      }
      fp.close();
    }
  }
  server.send(200, "text/plain", "OK!");
  ram_buf[0] = 0;
  send_ram();
  disp("00000");
  delay(2000);
  server.close();
  Serial.print("uptime=");
  Serial.print(millis());
  Serial.println("ms");
  Serial.println("reboot");
  Serial.flush();
  ESP.restart();
}
void AP() {
  // Go into software AP mode.
  struct softap_config cfgESP;
  disp("  AP ");
  ram_buf[0] = OTA_MODE; //ota
  send_ram();

  Serial.println("AP模式启动.\r\nssid:disp\r\npasswd:none");
  WiFi.mode(WIFI_AP);

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

  http_listen();
}
void http_listen() {

  server.on("/", handleRoot);
  server.on("/save.php", httpsave); //保存设置
  server.on("/generate_204", http204);//安卓上网检测

  server.onNotFound(http204);
  server.begin();
  Serial.println("HTTP服务器启动");
}
void http_loop() {
  server.handleClient();
}
uint32_t ms0;
void ap_loop() {
  dnsServer.processNextRequest();
  http_loop();
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
      Serial.print("batt:");
      Serial.print(v);
      Serial.print("V,millis()=");
      Serial.println(millis());
      Serial.println("power down");
      disp("00000");
      ht16c21_cmd(0x84, 0);
      server.close();
      ESP.reset();
      poweroff(9999); //关机
    }
  }
}
#endif //__AP_WEB_H__
