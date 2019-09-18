#include "global.h"
void setup() {
  set_gpio();
  setup_watchdog(WDTO_250MS); //睡8秒，醒一次
lora_init();
}
void loop() {
  char ch[10];
  lora_send_loop();
  sprintf(ch, "%02d %02d:%02d:%02d\r\n", day, hour, minute, sec);
  Serial.begin(9600);
  Serial.print(ch);
  Serial.end();
  power_down_8s();
//delay(100);
}
