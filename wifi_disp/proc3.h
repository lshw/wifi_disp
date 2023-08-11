#ifndef __PROC3_H__
#define __PROC3_H__
void proc3_setup() {
  WiFi.setAutoConnect(true);//自动链接上次
  wifi_station_connect();
  Serial.println(WiFi.localIP());
  init1();
  disp((char *)"P3  ");
  set_hostname();
  hello();
  get_value();
  wait_connected(10000);
  Serial.println(WiFi.localIP());
  if (WiFi_isConnected())
    udp_send(String(nvram.boot_count) + "," + String(millis()), (char *)"192.168.2.4", 8888, 8888);
  nvram.proc = PROC3_MODE;
  system_deep_sleep_set_option(2); //下次开机wifi不校准
  nvram.change = 1;
  poweroff(nvram.proc3_sec);
}
#endif //__PROC3_H__
