#ifndef __DHT_H__
#define __DHT_H__

extern float wendu, shidu;
uint8_t dht22_data[5];
uint32_t dht_next = 500;
#define DHT_PIN 0
bool dht() {
  // empty output data.
  pinMode(DHT_PIN, INPUT_PULLUP);
  if ( millis()  < dht_next) {
    dht_next = dht_next - millis();
    if (dht_next > 500) dht_next = 500;
    Serial.printf("delay(%d)\r\n", dht_next);
    delay(dht_next);
  }
  dht_next = millis() + 500;

  // According to protocol: http://akizukidenshi.com/download/ds/aosong/AM2302.pdf
  // notify DHT22 to start:
  //    1. T(be), PULL LOW 1ms(0.8-20ms).
  //    2. T(go), PULL HIGH 30us(20-200us), use 40us.
  //    3. SET TO INPUT or INPUT_PULLUP.
  pinMode(DHT_PIN, OUTPUT);
  digitalWrite(DHT_PIN, LOW);
  delayMicroseconds(10000);
  // Pull high and set to input, before wait 40us.
  // @see https://github.com/winlinvip/SimpleDHT/issues/4
  // @see https://github.com/winlinvip/SimpleDHT/pull/5
  digitalWrite(DHT_PIN, HIGH);
  pinMode(DHT_PIN, INPUT_PULLUP);
  delayMicroseconds(40);

  // DHT22 starting:
  //    1. T(rel), PULL LOW 80us(75-85us).
  //    2. T(reh), PULL HIGH 80us(75-85us).
  //levelTime(pin, LOW);
  //levelTime(pin, HIGH);
  if (pulseIn(DHT_PIN, HIGH, 200) > 120) return false;
  // DHT22 data transmite:
  //    1. T(LOW), 1bit start, PULL LOW 50us(48-55us).
  //    2. T(H0), PULL HIGH 26us(22-30us), bit(0)
  //    3. T(H1), PULL HIGH 70us(68-75us), bit(1)
  cli();
  for (int j = 0; j < 40; j++) {
    dht22_data[j / 8] = (dht22_data[j / 8] << 1) | (pulseIn(DHT_PIN, HIGH, 200) > 40 ? 1 : 0);    // specs: 22-30us -> 0, 70us -> 1
  }
  sei();
  if (
    ((dht22_data[0] + dht22_data[1] + dht22_data[2] + dht22_data[3]) & 0xff) == dht22_data[4]
    && ((dht22_data[0] + dht22_data[1] + dht22_data[2] + dht22_data[3] + dht22_data[4]) != 0)
  ) {
    wendu = 0.1 * ((dht22_data[2] & 0x7f) << 8 | dht22_data[3]);
    shidu = 0.1 * (dht22_data[0] << 8 | dht22_data[1]);
    if ((dht22_data[2] & 0x80 ) != 0)
      wendu = -wendu;
    Serial.printf("温度=%.1f, 湿度=%.1f%%\r\n", wendu, shidu);
    return true;
  } else {
    wendu = -999.0;
    shidu = -999.0;
    Serial.println(F("数据校验错"));
    return false;
  }
}
#endif
