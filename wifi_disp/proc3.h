#ifndef __PROC3_H__
#define __PROC3_H__
void proc3_setup() {
  WiFiUDP udp;
  float wendu0 = -300.0;
  int32_t qiya = -10e6;
  init1();
  disp((char *)" P3 ");
  set_hostname();
  hello();
  get_value();
  wait_connected(10000);
  fix_proc3_set();
  shan();
  nvram.nvram7 |= NVRAM7_CHARGE; //充电
  nvram.proc = PROC3_MODE;
  nvram.change = 1;
  save_nvram();
  system_deep_sleep_set_option(2); //下次开机wifi不校准
  Serial.flush();
  if (WiFi_isConnected()) {
    delay(100);
    if (!get_temp())
      wendu = -300.0;
    if (bmp.begin()) {
      if (wendu > -300.0)
        wendu0 = (wendu + bmp.readTemperature()) / 2;
      else
        wendu0 = bmp.readTemperature();
      qiya = bmp.readPressure();
      snprintf(disp_buf, sizeof(disp_buf), "%f", (float)bmp.readPressure() / 1000);
    } else {
      if (shidu >= 0.0 && shidu <= 100.0)
        snprintf_P(disp_buf, sizeof(disp_buf), PSTR("%02d-%02d"), wendu, shidu);
      else
        snprintf(disp_buf, sizeof(disp_buf), "%4.2f", wendu);
    }
    disp(disp_buf);
    wendu0 = wendu;
    String msg = String(nvram.boot_count) + ',' + String(millis()) + ',' + String(v, 2) + ',';
    if (wendu0 > -41)
      msg += String(wendu0) + ',';
    else
      msg += "-,";
    if (shidu >= 0.0 && shidu <= 100.0)
      msg += String(shidu) + ',';
    else
      msg += "-,";
    if (qiya > -10e6)
      msg += String(qiya);
    else
      msg += '-';

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
