#ifndef __PROC3_H__
#define __PROC3_H__
void proc3_setup() {
  WiFiUDP udp;
  WiFi.setAutoConnect(true);//自动链接上次
  wifi_station_connect();
  init1();
  disp((char *)"P3  ");
  if (bmp.begin()) {
    set_hostname();
    hello();
    get_value();
    wait_connected(10000);
    fix_proc3_set();
    nvram.proc = PROC3_MODE;
    system_deep_sleep_set_option(2); //下次开机wifi不校准
    nvram.change = 1;
    save_nvram();
    Serial.flush();
    if (WiFi_isConnected()) {
      String msg = String(nvram.boot_count) + ',' + String(millis()) + ',' + String((bmp.readTemperature() + wendu) / 2) + ',' + String(shidu) + ',' + String(bmp.readPressure());
      udp.begin(nvram.proc3_port);
      if (udp.beginPacket(nvram.proc3_host, nvram.proc3_port)
          || udp.beginPacket(nvram.proc3_host, nvram.proc3_port)
          || udp.beginPacket(nvram.proc3_host, nvram.proc3_port)) {
        if (udp.println(msg)
            || udp.println(msg)
            || udp.println(msg)) {
          udp.endPacket();
          Serial.println(F("send ok"));
        }
      }
      Serial.printf_P(PSTR("to %s:%d [%s]\r\n"), nvram.proc3_host, nvram.proc3_port, msg.c_str());
      Serial.flush();
    }
  }
  delay(300);
  poweroff(nvram.proc3_sec);
}
#endif //__PROC3_H__
