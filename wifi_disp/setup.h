#ifndef __SETUP_H__
#define __SETUP_H__
void setup_setup() {
  WiFi.setAutoConnect(true);//自动链接上次
  wifi_station_connect();
  init1();
  disp((char *)" OTA ");
  get_value();
  nvram.nvram7 |= NVRAM7_CHARGE; //充电
  nvram.change = 1;
  save_nvram();
  if (ds_pin == 0) { //v2.0
    if (nvram.have_lora > -5 && lora_init())
      lora_sleep();
  }
  wifi_setup();
  uint32_t ms = millis() + 10000;
  while (millis() < ms && !WiFi.isConnected())
    yield();
  if (wifi_station_get_connect_status() != STATION_GOT_IP) {
    ap_on_time = millis() + 30000;  //WPS 20秒
    if (WiFi.beginWPSConfig()) {
      delay(1000);
      uint8_t ap_id = wifi_station_get_current_ap_id();
      char wps_ssid[33], wps_password[65];
      memset(wps_ssid, 0, sizeof(wps_ssid));
      memset(wps_password, 0, sizeof(wps_password));
      struct station_config config[5];
      wifi_station_get_ap_info(config);
      strncpy(wps_ssid, (char *)config[ap_id].ssid, 32);
      strncpy(wps_password, (char *)config[ap_id].password, 64);
      config[ap_id].bssid_set = 1; //同名ap，mac地址不同
      wifi_station_set_config(&config[ap_id]); //保存成功的ssid,用于下次通讯
      wifi_set_add(wps_ssid, wps_password);
    }
  }
  if (wifi_station_get_connect_status() != STATION_GOT_IP) {
    AP();
    ota_status = 1;
    get_batt();
    if (power_in)
      ap_on_time = millis() + 1000000; //插AP模式1000秒
    else
      ap_on_time = millis() + 200000; //不插电AP模式200秒
  }
  httpd_listen();
  ota_setup();
}

void setup_loop() {
  if (ap_client_linked || connected_is_ok) {
    httpd_loop();
    ArduinoOTA.handle();
  }
  if (ap_client_linked)
    dnsServer.processNextRequest();
  if (connected_is_ok && !upgrading) { //连上AP
    if (!httpd_up) {
      ht16c21_cmd(0x88, 0); //不闪烁
      update_disp();
      zmd();
      httpd_up = true;
    }
  }
  if (run_zmd && !upgrading) {
    ht16c21_cmd(0x88, 0); //不闪烁
    run_zmd = false;
    zmd();
  }
  if (nvram.proc != GENERAL_MODE && millis() > 5000) { //OTA5秒后， 如果再重启， 就进入测温程序
    nvram.proc = GENERAL_MODE;
    nvram.change = 1;
    system_deep_sleep_set_option(2); //重启时不校准无线电
  }
}
#endif //__SETUP_H__
