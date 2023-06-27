bool power_in = true;
#define uS_TO_S_FACTOR 1000000ULL
#include "ht16c21.h"
RTC_DATA_ATTR uint16_t count;

  char disp_buf[15];
void setup() {
  ht16c21_setup();
  pinMode(3,OUTPUT);
  digitalWrite(3,HIGH);
  Serial.begin(115200);
  count++;
  snprintf(disp_buf, sizeof(disp_buf), "%05d", count);
  disp(disp_buf);
  Serial.println(disp_buf);
  Serial.print("ibat:");
  Serial.println(get_batt(2));
  Serial.print("ebat:");
  Serial.println(get_batt(1));
  Serial.flush();
  delay(1000);
  esp_sleep_enable_timer_wakeup(2 * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}
void loop() {
}

float get_batt(uint8_t adc) {//锂电池电压
  uint32_t dat = analogRead(adc);
  dat = analogRead(adc)
        + analogRead(adc)
        + analogRead(adc)
        + analogRead(adc)
        + analogRead(adc)
        + analogRead(adc)
        + analogRead(adc)
        + analogRead(adc);

    return (float) dat / 8 * (470.0 + 100.0) / 100.0 / 1023 /5.33*3.87;
}
