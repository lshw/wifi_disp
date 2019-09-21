#include "global.h"
uint8_t osc;
void setup() {
  set_gpio();
  power_adc_disable();
  power_twi_disable();
  power_timer1_disable();
  power_timer2_disable();
  Serial.begin(9600);
  osc=OSCCAL;
/* 用串口通讯，校准RC振荡器，
    for( int8_t i=-20;i<20;i++) {
    OSCCAL=osc+i;
    delay(10);
    Serial.print(F("offset="));
    Serial.println(i);
    Serial.flush();
    }
*/
  OSCCAL=osc-3;
  setup_watchdog(WDTO_4S); //睡8秒，醒一次
  lora_init();
Serial.println(bat());
}
void loop() {
  char ch[12];
  lora_send_loop();
  Serial.begin(9600);
  sprintf(ch, "%02d %02d:%02d:%02d ", day, hour, minute, sec);
  Serial.print(ch);
  Serial.println(bat());
  Serial.end();
  power_down();
}
