#ifndef __DS1820_H__
#define __DS1820_H__
#include <OneWire.h>
#include "global.h"
#define ds_pin 5
#define ds_vcc 4
OneWire  oneWire(ds_pin);

byte dsn[8]; //ds1820 sn
float temp;

bool ds_init() {
  temp = -999.0;
  pinMode(14, OUTPUT);
  digitalWrite(ds_vcc, LOW);
  delay(50);
  digitalWrite(ds_vcc, HIGH);
  delay(50);
  memset(dsn, 0, sizeof(dsn));
  oneWire.begin(ds_pin);
  oneWire.search(dsn);
  if (dsn[0] == 0) {
    Serial.println("没找到ds1820.");
    Serial.println();
    return false;
  }
  oneWire.reset();
  oneWire.skip(); //广播
  oneWire.write(0x44, 1);
  return true;
}

bool get_temp() {
  uint8_t i;
  bool ret = true;
  byte data[12];
  pinMode(12, INPUT);
  oneWire.reset();
  oneWire.select(dsn);
  oneWire.write(0xBE);         // Read Scratchpad
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = oneWire.read();
  }

  if (OneWire::crc8(data, 8) != data[8]) {
    temp = -999.0;
    if (ret == true) {
      ret = false;
    }

    // Convert the data to actual temperature
    // because the result is a 16 bit signed integer, it should
    // be stored to an "int16_t" type, which is always 16 bits
    // even when compiled on a 32 bit processor.
    int16_t raw = (data[1] << 8) | data[0];
    if (dsn[0] == 0x10) { //DS18S20
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
    temp = (float)raw / 16.0;
    if (temp == 85.0) {
      if (ret == true) {
        ret = false;
      }

    } else
      Serial.println("温度=" + String(temp));
  }
  if (ret == true) {
    digitalWrite(ds_vcc, LOW);
    digitalWrite(ds_pin, LOW);
  } else {
    oneWire.reset();
    oneWire.skip(); //广播
    oneWire.write(0x44, 1);//再读一次
  }
  return ret;
}

#endif //__DS1820_H__
