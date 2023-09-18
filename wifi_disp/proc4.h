#ifndef __PROC4_H__
#define __PROC4_H__
//lora转发到internet  通过wifi
void proc4_setup() {
  init1();
  if (power_in) {
    Serial.println(F("lora 网关模式"));
    disp(F("P4-L "));
    wifi_set_opmode(STATION_MODE);
    wifi_station_connect();
    set_hostname();
    hello();
    if (!wait_connected(30000)) {
      poweroff(10);
    }

  } else {
    Serial.println(F("lora 远端模式"));
    disp(F("P4-S "));
    lora_send_wendu();
    delay(100);
    poweroff(nvram.proc3_sec);
  }
}
#endif //__PROC4_H__
