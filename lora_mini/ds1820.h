#ifndef __DS1820_H__
#define __DS1820_H__
#include <OneWire.h>
#include "global.h"
#define ds_pin 5
#define ds_pullup 7
#define ds_vcc 6
OneWire  oneWire(ds_pin);

byte dsn[8]; //ds1820 sn
float temp = -999.0;

int8_t ds_status = 0; //0-关机 1-已上电 2-已发转换命令, 3~99-等待延迟, >100-延迟完成 <-5 不再初始化

void ds_init() { //上电， 如果是第一次上电， 就搜索ds1820
  if (ds_status > 0) return ;
  if (ds_status < -5) {
    digitalWrite(ds_vcc, LOW);
    digitalWrite(ds_pin, LOW);
    digitalWrite(ds_pullup, LOW);
    pinMode(ds_vcc, OUTPUT);
    pinMode(ds_pin, OUTPUT);
    pinMode(ds_pullup, INPUT);
    return false;
  }

  pinMode(ds_vcc, OUTPUT);
  pinMode(ds_pin, INPUT);
  pinMode(ds_pullup, INPUT);
  digitalWrite(ds_pin, LOW);
  digitalWrite(ds_pullup, LOW);
  digitalWrite(ds_vcc, LOW);
  delay(5);
  digitalWrite(ds_vcc, HIGH);
  digitalWrite(ds_pin, HIGH);
  delay(5);
  if (dsn[0] == 0) {
    oneWire.begin(ds_pin);
    oneWire.search(dsn);
    if (dsn[0] == 0) {
      Serial.println("没找到ds1820.");
      Serial.println();
      ds_status--;
    } else {
      Serial.print(F("找到ds1820 "));
      for (uint8_t i = 0; i < 8; i++) {
        if (dsn[i] < 0x10) Serial.write('0');
        Serial.print(dsn[i], HEX);
      }
      Serial.println();
    }
  }
  if (dsn[0] == 0)   return false;
  ds_status = 1;
  return true;
}


void ds_start() {
  if (ds_status < 1) {
    ds_init();
    return;
  }
  if (ds_status != 1) return;
  ds_status = 2;
  oneWire.reset();
  oneWire.skip(); //广播
  oneWire.write(0x44, 1);
}

bool get_temp() {
  uint8_t i;
  bool ret = true;
  byte data[12];
  if (ds_status < 100) return; //没有转换完成

  oneWire.reset();
  //oneWire.select(dsn);
  oneWire.skip(); //广播
  oneWire.write(0xBE, 1);        // Read Scratchpad
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = oneWire.read();
  }
  ds_status = 0;
  digitalWrite(ds_vcc, LOW);
  digitalWrite(ds_pin, LOW);
  digitalWrite(ds_pullup, LOW);
  pinMode(ds_pin, OUTPUT);

  if (OneWire::crc8(data, 8) != data[8]) {
    return false;
  } else {
    for (i = 0; i < 9; i++) {
      Serial.write(' ');
      if (data[i] < 0x10) Serial.write('0');
      Serial.print(data[i], HEX);
    }
    Serial.println();

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
  }
  return true;
}

#endif //__DS1820_H__
