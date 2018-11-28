#ifndef __DS1820_H__
#define __DS1820_H__
#include <OneWire.h>
OneWire  ds(12);
float temp = -999;

byte dsn[9]; //ds1820 sn
char key[19]; //ds1820 sn 字符串

bool ds_init() {
  uint8_t i;
  pinMode(14, OUTPUT);
  digitalWrite(14, LOW);
  i=digitalRead(12);
  digitalWrite(12,LOW);
  delay(100);
  digitalWrite(14,HIGH);
  digitalWrite(12,i);
  delay(1);
  if ( !ds.search(dsn)) {
    Serial.println("没找到ds1820.");
    Serial.println();
    ds.reset_search();
    return false;
  }
  dsn[8] = 0;
  Serial.print("DS1820 =");
  sprintf(key, "%02x%02x%02x%02x%02x%02x%02x%02x", dsn[0], dsn[1], dsn[2], dsn[3], dsn[4], dsn[5], dsn[6], dsn[7]);
  Serial.println(key);
  if (OneWire::crc8(dsn, 7)!=dsn[7]) return false;
  ds.reset();
  ds.select(dsn);
  ds.write(0x44, 1);
  return true;
}

float get_temp() {
  uint8_t i;
  byte data[12];

  Serial.print("\r\nDS1820 Data = ");
  ds.reset();
  ds.select(dsn);
  ds.write(0xBE);         // Read Scratchpad
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    Serial.print(" ");
    if (data[i] < 0x10) Serial.write('0');
    Serial.print(data[i], HEX);
  }
  Serial.println();

  if (OneWire::crc8(data, 8) != data[8])
    return -999.0;

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
  float temp = (float)raw / 16.0;
  Serial.print("温度=");
  Serial.println(temp);
  return temp;
}

#endif //__DS1820_H__

