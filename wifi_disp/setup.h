#ifndef __SETUP_H__
#define __SETUP_H__
void setup_setup() {
  init1();
  disp(F("SETUP"));
  nvram.nvram7 |= NVRAM7_CHARGE; //充电
  nvram.change = 1;
  save_nvram();
  pcb_ver_detect();
  if (nvram.have_dht == 1) {
    dht_();
  } else {
    get_value();
  }
  if (nvram.pcb_ver > 0) { //v2.0, v3.0
    if (nvram.have_lora > -5 && lora_init())
      lora_sleep();
  }
  add_limit_millis();
  _myTicker.attach(1, timer1s);
  wait_connected(10000); //等待连接
  httpd_listen();
  ota_setup();
  if (!WiFi.localIP()) {
    wifi_config();
  }
}

bool httpd_up = false;
void setup_loop() {
  if (ap_client_linked || connected_is_ok) {
    httpd.handleClient();
    ArduinoOTA.handle();
  }
  if (ap_client_linked)
    dnsServer.processNextRequest();
  if (!upgrading && WiFi_isConnected()) { //连上AP
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
  add_limit_millis();
}
#endif //__SETUP_H__
