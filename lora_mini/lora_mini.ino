#include "global.h"
void setup() {
  set_gpio();
  Serial.begin(9600);
  /*
    uint8_t osc;
    osc=OSCCAL;
    for( int8_t i=-20;i<20;i++) {
    OSCCAL=osc+i;
    delay(10);
    Serial.print(F("offset="));
    Serial.println(i);
    }
    OSCCAL=osc-5;
  */
  setup_watchdog(WDTO_1S); //睡8秒，醒一次
  lora_init();
}
void loop() {
  char ch[10];
  lora_send_loop();
  sprintf(ch, "%02d %02d:%02d:%02d\r\n", day, hour, minute, sec);
  Serial.print(ch);
  power_down_8s();
  //delay(100);
}
