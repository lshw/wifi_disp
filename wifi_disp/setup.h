#ifndef __SETUP_H__
#define __SETUP_H__
void setup_setup() {
  init1();
  disp((char *)"SETUP");
  nvram.nvram7 |= NVRAM7_CHARGE; //充电
  nvram.change = 1;
  save_nvram();
  pcb_ver_detect();
  get_value();
  if (nvram.pcb_ver > 0) { //v2.0, v3.0
    if (nvram.have_lora > -5 && lora_init())
      lora_sleep();
  }
  _myTicker.attach(1, timer1s);
  wait_connected(10000); //等待连接
  if (!WiFi.localIP()) {
    wifi_config();
  }
  httpd_listen();
  ota_setup();
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
  if (nvram.proc != GENERAL_MODE && millis() > 5000) { //OTA5秒后， 如果再重启， 就进入测温程序
    nvram.proc = GENERAL_MODE;
    nvram.change = 1;
    system_deep_sleep_set_option(2); //重启时不校准无线电
  }
}
#endif //__SETUP_H__
