#ifndef __PROC3_H__
#define __PROC3_H__
void proc3_setup() {
  WiFi.setAutoConnect(true);//自动链接上次
  wifi_station_connect();
  init1();
  wifi_setup();
  disp((char *)"P3  ");
  get_value();
  uint32_t ms0;
  ms0 = millis() + 10000;
  while (ms0 > millis() && !WiFi.isConnected()) {
    yield();
  }
  if (WiFi.isConnected())
    udp_send(String(millis()), (char *)"192.168.2.4", 8888, 8888);
  nvram.proc = PROC3_MODE;
  system_deep_sleep_set_option(2); //下次开机wifi不校准
  nvram.change = 1;
  poweroff(nvram.proc3_sec);
}
#endif //__PROC3_H__
