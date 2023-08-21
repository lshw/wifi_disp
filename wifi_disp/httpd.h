#ifndef __AP_WEB_H__
#define __AP_WEB_H__
#ifdef CONFIG_IDF_TARGET_ESP32C3
#include <WebServer.h>
#else
#include <ESP8266WebServer.h>
#endif
#include <ArduinoOTA.h>
#include "wifi_client.h"
#include <Wire.h>
extern void disp(char *);
extern uint8_t lora_version;
void poweroff(uint32_t);
float get_batt();
void ht16c21_cmd(uint8_t cmd, uint8_t dat);
String body;
ESP8266WebServer httpd(80);
String select_option(uint8_t value, uint8_t dat, String str) {
  String selected = "";
  if (value == dat) selected = " selected";
  return F("<option value=") + String(value) + selected + ">" + str + F("</option>");
}
String lora_set() {
  if (lora_version == 255 || lora_version == 0) return "";
  return F("<hr>"
           "lora扩频调制带宽:<select name=lora_bw>")
         + select_option(0x00, nvram.bw, String(F("LR_BW_7p8k")))
         + select_option(0x10, nvram.bw, String(F("LR_BW_10p4k")))
         + select_option(0x20, nvram.bw, String(F("LR_BW_15p6k")))
         + select_option(0x30, nvram.bw, String(F("LR_BW_20p8k")))
         + select_option(0x40, nvram.bw, String(F("LR_BW_31p25k")))
         + select_option(0x50, nvram.bw, String(F("LR_BW_41p7k")))
         + select_option(0x60, nvram.bw, String(F("LR_BW_62p5k")))
         + select_option(0x70, nvram.bw, String(F("LR_BW_125k")))
         + select_option(0x80, nvram.bw, String(F("LR_BW_250k")))
         + select_option(0x90, nvram.bw, String(F("LR_BW_500k")))
         + F("</select>&nbsp;"
             "lora纠错率:<select name=lora_cr>")
         + select_option(0x02, nvram.cr, String(F("LR_CODINGRATE_1p25")))
         + select_option(0x04, nvram.cr, String(F("LR_CODINGRATE_1p5")))
         + select_option(0x06, nvram.cr, String(F("LR_CODINGRATE_1p75")))
         + select_option(0x08, nvram.cr, String(F("LR_CODINGRATE_2")))
         + F("</select>&nbsp;"
             "lora扩频因子:<select name=lora_sf>")
         + select_option(0x60, nvram.sf, String(F("LR_SPREADING_FACTOR_6")))
         + select_option(0x70, nvram.sf, String(F("LR_SPREADING_FACTOR_7")))
         + select_option(0x80, nvram.sf, String(F("LR_SPREADING_FACTOR_8")))
         + select_option(0x90, nvram.sf, String(F("LR_SPREADING_FACTOR_9")))
         + select_option(0xa0, nvram.sf, String(F("LR_SPREADING_FACTOR_10")))
         + select_option(0xb0, nvram.sf, String(F("LR_SPREADING_FACTOR_11")))
         + select_option(0xc0, nvram.sf, String(F("LR_SPREADING_FACTOR_12")))
         + F("</select>");
}
void httpd_send_200(String javascript) {
  httpd.sendHeader( "charset", "utf-8" );
  httpd.send(200, "text/html", "<html>"
             "<head>"
             "<title>" + hostname + " " + GIT_VER + "</title>"
             "<meta http-equiv=Content-Type content='text/html;charset=utf-8'>"
             "<script>"
             "function gotoif(url)"
             "{"
             "if (confirm('确定?')) {"
             "location.replace(url);"
             "}"
             "}"

             "function modi(url,text,Defaulttext) {"
             "var data=prompt(text,Defaulttext);"
             "if (data==null) {return false;}"
             "location.replace(url+data);"
             "}"
             + javascript +
             "</script>"
             "</head>"
             "<body>"
             + body +
             "</body>"
             "</html>");
  httpd.client().stop();
}

void http204() {
  httpd.send(204, "", "");
  httpd.client().stop();
}
void http_proc3() {
  String str = "";
  fix_proc3_set();
  add_limit_millis();
  body = "<h1>其他设置</h1>"
         "<hr>"
         "<a href=/><button>返回设置</button></a>"
         "<hr>"
         "PROC_3 测试间隔(10-250): <span onclick=modi('/save.php?proc3_sec=','修改测试间隔','" + String(nvram.proc3_sec) + "')><font color = blue>" + String(nvram.proc3_sec) + "</font>秒</span>"
         "&nbsp;&nbsp;udp服务器: <span onclick=modi('/save.php?proc3_host=','修改服务器,最长32字符','" + String(nvram.proc3_host) + "')><font color = blue>" + String(nvram.proc3_host) + "</font></span>"
         "&nbsp;&nbsp;udp端口: <span onclick=modi('/save.php?proc3_port=','修改服务器端口,1025-65535','" + String(nvram.proc3_port) + "')><font color = blue>" + String(nvram.proc3_port) + "</font></span><hr>";
  httpd_send_200("");
}
void handleRoot() {
  String wifi_stat, wifi_scan, i2c_scan;
  String ssid;
  add_limit_millis();
  for (uint8_t i = 0; i < httpd.args(); i++) {
    if (httpd.argName(i).compareTo("i2c_scan") == 0) {
      Wire.begin();
      for (uint16_t i0 = 1 ; i0 < 0x80; i0++) {
        Wire.beginTransmission(i0);
        if (Wire.endTransmission() == 0) {
          switch (i0) {
            case 0x38:
              Serial.println(F("0x38:LCD控制器"));
              i2c_scan += "<br>0x38:lcd控制器";
              break;
            case 0x44:
              Serial.println(F("0x44:SHT44温湿度探头"));
              i2c_scan += "<br>0x44:SHT44温湿度探头";
              break;
            case 0x77:
              bmp.begin(); //需要安装Adafruit BMP085 Library 库
              Serial.println(F("0x77:BMP180气压探头"));
              i2c_scan += "<br>0x77:BMP180气压探头  温度:" + String(bmp.readTemperature(), 1)
                          + "摄氏度, 气压:" + String(bmp.readPressure())
                          + "帕, 海拔:" + String(bmp.readAltitude(), 0) + "米";
              break;
            default:
              Serial.printf_P(PSTR("0x%02x:未知设备\r\n"), i0);
              i2c_scan += "<br>0x" + String(i0, HEX) + ":未知设备";
          }
        }
      }
    } else if (httpd.argName(i).compareTo("wifi_scan") == 0) {
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
    }
  }
  if (wifi_scan == "") {
    wifi_scan = "<a href=/?wifi_scan=1><buttom>扫描WiFi</buttom></a>";
  }
  if (i2c_scan == "") {
    wifi_scan += "<hr><a href=/?i2c_scan=1><buttom>扫描i2c设备</buttom></a>";
  } else
    wifi_scan += "<hr>找到i2c设备:" + i2c_scan;

  yield();
  if (connected_is_ok) {
    wifi_stat = "wifi已连接 ssid:<mark>" + String(WiFi.SSID()) + "</mark> &nbsp; "
                + "ap:<mark>" + WiFi.BSSIDstr() + "</mark> &nbsp; "
                + "信号:<mark>" + String(WiFi.RSSI()) + "</mark>dbm &nbsp; "
                + "ip:<mark>" + WiFi.localIP().toString() + "</mark> &nbsp; "
                + "电池电压:<mark>" + String(v) + "</mark>V &nbsp; ";
  }
  if ( shidu >= 0.0 && shidu <= 100.0)
    wifi_stat += "湿度:<mark>" + String((int8_t)shidu) + "%</mark> &nbsp; ";
  if (wendu > -300.0)
    wifi_stat += "温度:<mark>" + String(wendu) + "</mark>&#8451<br>";
  body = "SN:<mark>" + hostname + "</mark> &nbsp; "
         "硬件版本:<mark>" + String(nvram.pcb_ver) + "</mark> &nbsp; "
         "软件版本:<mark>" VER "</mark>"
         "&nbsp;<a href=/proc3.php><button>其它设置</button></a>"
         "<hr>"
         + wifi_stat + "<hr>" + wifi_scan +
         "<hr><form action=/save.php method=post>"
         "输入ssid:passwd(可以多行多个)"
         "<input type=submit value=save><br>"
         "<textarea  style='width:500px;height:80px;' name=data>" + get_ssid() + "</textarea><br>"
         "可以设置自己的服务器地址(清空恢复)<br>"
         "url0:<input maxlength=100  size=30 type=text value='" + get_url(0) + "' name=url><br>"
         "url1:<input maxlength=100  size=30 type=text value='" + get_url(1) + "' name=url1><br>"
         + lora_set()
         + "<hr><input type=submit name=submit value=save>"
         "&nbsp;<input type=submit name=reboot value='reboot'>"
         "</form>"
         "<hr>"
         "<form method='POST' action='/update.php' enctype='multipart/form-data'>上传更新固件firmware:<input type='file' name='update'><input type='submit' value='Update'></form>"
         "<hr><table width=100%><tr><td align=left width=50%>程序源码:<a href=https://github.com/lshw/wifi_disp/releases/tag/V"  GIT_VER " target=_blank>https://github.com/lshw/wifi_disp/release/tag/V" GIT_VER "</a><td><td align=right width=50%>程序编译时间: <mark>" __DATE__ " " __TIME__ "</mark></td</tr></table><br>编译参数:[" BUILD_SET "] GCC" + String(__GNUC__) + "." + String(__GNUC_MINOR__)
         + "<hr>";
  httpd_send_200("<script>"
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
                 "</script>");
}
void handleNotFound() {
  File fp;
  int ch;
  String message;
  add_limit_millis();
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
  message = "404 File Not Found\n\n";
  message += "URI: ";
  message += httpd.uri();
  message += "<br><a href=/?" + String(millis()) + "><button>点击进入首页</button></a>";
  httpd.send(200, "text/html", "<html>"
             "<head>"
             "<meta http-equiv=Content-Type content='text/html;charset=utf-8'>"
             "</html>"
             "<body>" + message + "</body></html>");
  httpd.client().stop();
  message = "";
}
void http_add_ssid() {
  String data;
  char ch;
  int8_t mh_offset;
  for (uint8_t i = 0; i < httpd.args(); i++) {
    if (httpd.argName(i).compareTo("data") == 0) {
      data = httpd.arg(i);
      data.trim();
      data.replace("\xef\xbc\x9a", ":"); //utf8 :
      data.replace("\xa3\xba", ":"); //gbk :
      data.replace("\xa1\x47", ":"); //big5 :
      break;
    }
  }
  if (data == "") return;
  mh_offset = data.indexOf(':');
  if (mh_offset < 2) return;

  wifi_set_add(data.substring(0, mh_offset).c_str(), data.substring(mh_offset + 1).c_str());
  httpd.send(200, "text/html", "<html><head></head><body><script>location.replace('/?" + String(millis()) + "');</script></body></html>");
  yield();
}
void httpsave() {
  File fp;
  String url, data;
  bool nvram_update = false;
  SPIFFS.begin();
  bool reboot_now = false;
  add_limit_millis();
  for (uint8_t i = 0; i < httpd.args(); i++) {
    if (httpd.argName(i).compareTo("proc3_sec") == 0) {
      nvram.proc3_sec = httpd.arg(i).toInt();
      if (nvram.proc3_sec < 10)
        nvram.proc3_sec = 10;
      nvram.change = 1;
      save_nvram();
      nvram_update = true;
      continue;
    }
    if (httpd.argName(i).compareTo("proc3_port") == 0) {
      nvram.proc3_port = httpd.arg(i).toInt();
      if (nvram.proc3_port < 1025)
        nvram.proc3_port = 1025;
      nvram.change = 1;
      save_nvram();
      nvram_update = true;
      continue;
    }
    if (httpd.argName(i).compareTo("proc3_host") == 0) {
      data = httpd.arg(i);
      data.trim();
      memset(nvram.proc3_host, 0, sizeof(nvram.proc3_host));
      strncpy(nvram.proc3_host, data.substring(0, sizeof(nvram.proc3_host) - 1).c_str(), sizeof(nvram.proc3_host) - 1);
      nvram.change = 1;
      save_nvram();
      nvram_update = true;
      continue;
    }
    if (httpd.argName(i).compareTo("reboot") == 0) {
      reboot_now = true;
      continue;
    }
    if (httpd.argName(i).compareTo("data") == 0) {
      data = httpd.arg(i);
      data.trim();
      data.replace("\xef\xbc\x9a", ":"); //utf8 :
      data.replace("\xa3\xba", ":"); //gbk :
      data.replace("\xa1\x47", ":"); //big5 :
      if (data.length() > 8) {
        Serial.printf_P(PSTR("data:[%s]\r\n"), data.c_str());
        fp = SPIFFS.open("/ssid.txt", "w");
        fp.println(data);
        fp.close();
        fp = SPIFFS.open("/ssid.txt", "r");
        Serial.print(F("保存wifi设置到文件/ssid.txt "));
        Serial.print(fp.size());
        Serial.println(F("字节"));
        fp.close();
      } else if (data.length() < 2)
        SPIFFS.remove("/ssid.txt");
    } else if (httpd.argName(i).compareTo("url") == 0) {
      url = httpd.arg(i);
      url.trim();
      if (url.length() == 0) {
        Serial.println(F("删除url0设置"));
        SPIFFS.remove("/url.txt");
      } else {
        Serial.printf_P(PSTR("url0:[%s]\r\n"), url.c_str());
        fp = SPIFFS.open("/url.txt", "w");
        fp.println(url);
        fp.close();
      }
    } else if (httpd.argName(i).compareTo(F("lora_bw")) == 0) {
      nvram.bw = httpd.arg(i).toInt();
      nvram.bw = nvram.bw & 0xf0;
      if (nvram.bw > LR_BW_500k) nvram.bw = LR_BW_500k;
      nvram.change = 1;
      save_nvram();
      nvram_update = true;
    } else if (httpd.argName(i).compareTo(F("lora_cr")) == 0) {
      nvram.cr = httpd.arg(i).toInt();
      nvram.cr &= 0xe;
      if (nvram.cr < LR_CODINGRATE_1p25) nvram.cr = LR_CODINGRATE_1p25;
      if (nvram.cr > LR_CODINGRATE_2) nvram.cr = LR_CODINGRATE_2;
      nvram.change = 1;
      save_nvram();
      nvram_update = true;
    } else if (httpd.argName(i).compareTo(F("lora_sf")) == 0) {
      nvram.sf = httpd.arg(i).toInt() & 0xf0;
      if (nvram.sf < LR_SPREADING_FACTOR_6)
        nvram.sf = LR_SPREADING_FACTOR_6;
      if (nvram.sf > LR_SPREADING_FACTOR_12)
        nvram.sf = LR_SPREADING_FACTOR_12;
      nvram.change = 1;
      save_nvram();
      nvram_update = true;
    } else if (httpd.argName(i).compareTo("url1") == 0) {
      url = httpd.arg(i);
      url.trim();
      if (url.length() == 0) {
        Serial.println(F("删除url1设置"));
        SPIFFS.remove("/url1.txt");
      } else {
        Serial.printf_P(PSTR("url1:[%s]\r\n"), url.c_str());
        fp = SPIFFS.open("/url1.txt", "w");
        fp.println(url);
        fp.close();
      }
    }
  }
  url = "";
  if (nvram_update) {
    fp = SPIFFS.open("/nvram.bin", "w");
    fp.write((char *) &nvram, sizeof(nvram));
    fp.close();
  }
  SPIFFS.end();
  httpd.send(200, "text/html", "<html><head></head><body><script>location.replace('/');</script></body></html>");
  yield();
  if (reboot_now) {
    nvram.proc = GENERAL_MODE;
    nvram.change = 1;
    save_nvram();
    ESP.restart();
  }
}
void httpd_listen() {

  httpd.on("/", handleRoot);
  httpd.on("/proc3.php", http_proc3);
  httpd.on("/save.php", httpsave); //保存设置
  httpd.on("/add_ssid.php", http_add_ssid); //保存设置
  httpd.on("/generate_204", http204);//安卓上网检测

  httpd.on("/update.php", HTTP_POST, []() {
    if (nvram.proc != GENERAL_MODE) {
      nvram.proc = GENERAL_MODE;
      nvram.change = 1;
      save_nvram();
    }
    httpd.sendHeader("Connection", "close");
    if (Update.hasError()) {
      upgrading = false;
      Serial.println(F("上传失败"));
      httpd.send(200, "text/html", "<html>"
                 "<head>"
                 "<meta http-equiv=Content-Type content='text/html;charset=utf-8'>"
                 "</head>"
                 "<body>"
                 "升级失败 <a href=/>返回</a>"
                 "</body>"
                 "</html>"
                );
    } else if (crc.finalize() == CRC_MAGIC) {
      httpd.send(200, "text/html", "<html>"
                 "<head>"
                 "<meta http-equiv=Content-Type content='text/html;charset=utf-8'>"
                 "</head>"
                 "<body>"
                 "<script>setTimeout(function(){ alert('升级成功!'); }, 15000); </script>"
                 "</body>"
                 "</html>"
                );
      Serial.println(F("上传成功"));
      Serial.flush();
      ht16c21_cmd(0x88, 1); //闪烁
      delay(5);
      ESP.restart();
    } else {
      body = "升级失败 <a href=/><buttom>返回首页</buttom></a>";
      httpd_send_200("");
    }
    yield();
  }, []() {
    HTTPUpload& upload = httpd.upload();
    if (upload.status == UPLOAD_FILE_START) {
      ht16c21_cmd(0x88, 0); //停闪烁
      Serial.setDebugOutput(true);
      WiFiUDP::stopAll();
      Serial.printf_P(PSTR("Update: %s\r\n"), upload.filename.c_str());
      uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
      if (!Update.begin(maxSketchSpace)) { //start with max available size
        Update.printError(Serial);
      }
      crc.reset();
      upgrading = true;
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      crc.update((uint8_t *)upload.buf, upload.currentSize);
      snprintf_P(disp_buf, sizeof(disp_buf), PSTR("UP.%3d"), upload.totalSize / 1000);
      disp(disp_buf);
      Serial.printf_P(PSTR("size:%ld\r\n"), (uint32_t)upload.totalSize);
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        if (crc.finalize() != CRC_MAGIC)
          Serial.printf_P(PSTR("File Update : %u\r\nCRC32 error ...\r\n"), upload.totalSize);
        else
          Serial.printf_P(PSTR("Update Success: %u\r\nRebooting...\r\n"), upload.totalSize);
      } else {
        Update.printError(Serial);
      }
      Serial.setDebugOutput(false);
    }
    yield();
  });
  httpd.onNotFound(handleNotFound);
  httpd.begin();

  Serial.printf_P(PSTR("HTTP服务器启动! 用浏览器打开 http://%s.local\r\n"), hostname.c_str());
}

void ota_loop() {

}
#endif //__AP_WEB_H__
