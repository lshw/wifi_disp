#ifndef __DHT_H__
#define __DHT_H__
long levelTime(byte level, int firstWait, int interval) {
  unsigned long time_start = micros();
  long time = 0;
  bool loop = true;
  for (int i = 0 ; loop; i++) {
    if (time < 0 || time > levelTimeout) {
      return -1;
    }
    time = micros() - time_start;
    loop = (digitalRead(pin) == level);
  }
  return time;
}
uint8_t dht22_data[5];
bool dht(uint8_t pin) {
  // empty output data.
  memset(dht22_data, 0, 5);

  // According to protocol: http://akizukidenshi.com/download/ds/aosong/AM2302.pdf
  // notify DHT22 to start:
  //    1. T(be), PULL LOW 1ms(0.8-20ms).
  //    2. T(go), PULL HIGH 30us(20-200us), use 40us.
  //    3. SET TO INPUT or INPUT_PULLUP.
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  delayMicroseconds(1000);
  // Pull high and set to input, before wait 40us.
  // @see https://github.com/winlinvip/SimpleDHT/issues/4
  // @see https://github.com/winlinvip/SimpleDHT/pull/5
  digitalWrite(pin, HIGH);
  pinMode(pin, INPUT_PULLUP);
  delayMicroseconds(40);

  // DHT22 starting:
  //    1. T(rel), PULL LOW 80us(75-85us).
  //    2. T(reh), PULL HIGH 80us(75-85us).
  long t = 0;
  if ((t = levelTime(LOW)) < 30) {
    return false;
  }
  if ((t = levelTime(HIGH)) < 50) {
    return false;
  }

  // DHT22 data transmite:
  //    1. T(LOW), 1bit start, PULL LOW 50us(48-55us).
  //    2. T(H0), PULL HIGH 26us(22-30us), bit(0)
  //    3. T(H1), PULL HIGH 70us(68-75us), bit(1)
  for (int j = 0; j < 40; j++) {
    t = levelTime(LOW);          // 1.
    if (t < 24) {                    // specs says: 50us
      return false;
    }

    // read a bit
    t = levelTime(HIGH);              // 2.
    if (t < 11) {                     // specs say: 26us
      return false;
    }
    dht22_data[j / 8] = (dht22_data[j / 8] << 1) | (t > 40 ? 1 : 0);     // specs: 22-30us -> 0, 70us -> 1
  }

  // DHT22 EOF:
  //    1. T(en), PULL LOW 50us(45-55us).
  t = levelTime(LOW);
  if (t < 24) {
    return false;
  }

  return dht22_data[0] + dht22_data[1] + dht22_data[2] + dht22_data[3] == dht22_data[4];
}
#endif
