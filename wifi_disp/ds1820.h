#ifndef __DS1820_H__
#define __DS1820_H__
#include "OneWire.h"
OneWire  oneWire(12);
float wendu = -300, shidu = 101.0;
byte dsn[32][8]; //ds1820 sn
float temp[32];

uint32_t last_load_temp = 0;
bool ds_init() {
  uint8_t i;
  File fp;
  bool save = false;
  char key[17];
  temp[0] = -999.0;
  if (nvram.ds18b20_pin == 12) { //pcb_ver0 18b20需要gpio14供电
    pinMode(14, OUTPUT);
    digitalWrite(14, HIGH);
    delay(50);
  }
  memset(dsn, 0, sizeof(dsn));
  oneWire.begin(nvram.ds18b20_pin);
  if (oneWire.search(dsn[0])) {
    Serial.printf_P(PSTR("DS18B20[%d]="), i);
    snprintf_P(key, sizeof(key), PSTR("%02x%02x%02x%02x%02x%02x%02x%02x"), dsn[i][0], dsn[i][1], dsn[i][2], dsn[i][3], dsn[i][4], dsn[i][5], dsn[i][6], dsn[i][7]);
    Serial.println(key);
  } else
    return false;
  for (; i < 32; i++) {
    if (!oneWire.search(dsn[i]))
      break;
    temp[i] = -999.0;
    Serial.printf_P(PSTR("DS18B20[%d]="), i);
    snprintf_P(key, sizeof(key), PSTR("%02x%02x%02x%02x%02x%02x%02x%02x"), dsn[i][0], dsn[i][1], dsn[i][2], dsn[i][3], dsn[i][4], dsn[i][5], dsn[i][6], dsn[i][7]);
    Serial.println(key);
  }
  if (dsn[0][0] == 0) {
    Serial.println(F("没找到ds1820.\r\n"));
    SPIFFS.end();
    return false;
  }
  oneWire.reset();
  oneWire.skip(); //广播
  oneWire.write(0x44, 1);
  if (dsn[1][0] != 0) { //有多个探头时，外接探头是信号线供电， 测温期间，要对12进行上拉。
    pinMode(nvram.ds18b20_pin, OUTPUT);
    digitalWrite(nvram.ds18b20_pin, HIGH);
  }
  return true;
}
bool get_temp() {
  uint8_t i, n;
  if (nvram.ds18b20_pin < 0) return false;
  if (last_load_temp + 200 > millis()) return false;
  last_load_temp = millis();
  bool ret = true;
  byte data[12];
  pinMode(12, INPUT);
  for (n = 0; n < 32; n++) {
    if (dsn[n][0] == 0) continue;
    if (temp[n] > -300 & temp[n] != 85.00) continue;
    oneWire.reset();
    oneWire.select(dsn[n]);
    oneWire.write(0xBE);         // Read Scratchpad
    for ( i = 0; i < 9; i++) {           // we need 9 bytes
      data[i] = oneWire.read();
    }

    if (OneWire::crc8(data, 8) != data[8]) {
      temp[n] = -999.0;
      if (ret == true) {
        ret = false;
      }
      continue;
    }

    // Convert the data to actual temperature
    // because the result is a 16 bit signed integer, it should
    // be stored to an "int16_t" type, which is always 16 bits
    // even when compiled on a 32 bit processor.
    int16_t raw = (data[1] << 8) | data[0];
    if (dsn[n][0] == 0x10) { //DS18S20
      raw = raw << 3; // 9 bit resolution default
      if (data[7] == 0x10) {
        // "count remain" gives full 12 bit resolution
        raw = (raw & 0xFFF0) + 12 - data[6];
      }
    } else { //DS18B20 or DS1822
      byte cfg = (data[4] & 0x60);
      // at lower res, the low bits are undefined, so let's zero them
      if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
      else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
      else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
      //// default is 12 bit resolution, 750 ms conversion time
    }
    temp[n] = (float)raw / 16.0;
    if (temp[n] == 85.0) {
      if (ret == true) {
        ret = false;
      }

    } else {
      Serial.printf_P(PSTR("温度%d="), n);
      Serial.println(temp[n]);
    }
  }
  if (ret == true) {
    digitalWrite(nvram.ds18b20_pin, LOW);
  } else {
    oneWire.reset();
    oneWire.skip(); //广播
    oneWire.write(0x44, 1);//再读一次
    if (dsn[1][0] != 0) { //有多个探头时，外接探头是信号线供电， 测温期间，要对12进行上拉。
      pinMode(nvram.ds18b20_pin, OUTPUT);
      digitalWrite(nvram.ds18b20_pin, HIGH);
    }
  }
  wendu = temp[0];
  shidu = 101.0;
  return ret;
}

#endif //__DS1820_H__
