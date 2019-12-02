#include "global.h"
uint8_t osc;
void setup() {
  power_adc_disable();
  power_twi_disable();
  power_timer1_disable();
  power_timer2_disable();
  set_gpio();
  Serial.begin(9600);
  osc = OSCCAL;
  pinMode(A4, INPUT);
  digitalWrite(A4, LOW); //充电
  /*
      for( int8_t i=-20;i<20;i++) {
      OSCCAL=osc+i;
      delay(10);
      Serial.print(F("offset="));
      Serial.println(i);
      Serial.flush();
      }
  */
  OSCCAL = osc - 5;
  memset(dsn, 0, sizeof(dsn));
  setup_watchdog(WDTO_8S); //睡8秒，醒一次
  lora_init();
  ds_init();
  ds_start();
}
uint8_t last_min;
void loop() {
  char ch[12];
  if (minute / 30 != last_min) {
    bat();
    last_min = minute / 30;
    ds_init();
    ds_start();//半小时测温一次
  }
  //  sprintf(ch, "%02d %02d:%02d:%02d ", day, hour, minute, sec);
  //  Serial.print(ch);
  //  Serial.print(bat());
  //  Serial.print(F(",ds_status="));
  //  Serial.print(ds_status);
  //  Serial.print(F(",temp="));
  //  Serial.println(temp);
  //  Serial.end();
  power_down();
  if (ds_status == 2) ds_status = 100;
  //  Serial.begin(9600);
  if (ds_status >= 100) { //测温完成
    get_temp();
    lora_send_loop();
  }
}
