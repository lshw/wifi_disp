#ifndef __PROC3_H__
#define __PROC3_H__
void proc3_setup() {
  WiFiUDP udp;
  String msg;
  set_hostname();
  pcb_ver_detect();
  if (nvram.have_dht) {
    if (wendu < -299.0)
      dht_();
    wifi_set_opmode(STATION_MODE);
    wifi_station_connect();
  }
  _myTicker.attach(1, timer1s);
  wait_connected(10000);
  nvram.nvram7 |= NVRAM7_CHARGE; //充电
  nvram.proc = PROC3_MODE;
  nvram.change = 1;
  save_nvram();
  if (nvram.have_dht) {
    if (wendu < -299.0) {
      if (!dht_()) poweroff(1);
    }
  }
  get_value();
  fix_proc3_set();
  shan();
  system_deep_sleep_set_option(2); //下次开机wifi不校准
  Serial.flush();
  if (WiFi_isConnected()) {
    delay(100);
    msg = val_str();
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
  delay(100);
  poweroff(nvram.proc3_sec);
}
#endif //__PROC3_H__
