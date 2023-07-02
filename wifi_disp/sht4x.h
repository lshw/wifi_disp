#ifndef __SHT4X_H__
#define __SHT4X_H__
//sht4x 温湿度探头 ， i2c接口
extern uint8_t temp_data[6];
extern float wendu, shidu;
uint8_t crc8(const uint8_t *data, int len) {
  /*

     CRC-8 formula from page 14 of SHT spec pdf

     Test data 0xBE, 0xEF should yield 0x92

     Initialization data 0xFF
     Polynomial 0x31 (x8 + x5 +x4 +1)
     Final XOR 0x00
  */

  const uint8_t POLYNOMIAL(0x31);
  uint8_t crc(0xFF);

  for (int j = len; j; --j) {
    crc ^= *data++;

    for (int i = 8; i; --i) {
      crc = (crc & 0x80) ? (crc << 1) ^ POLYNOMIAL : (crc << 1);
    }
  }
  return crc;
}
bool sht4x_cmd(uint8_t cmd) {
  Wire.beginTransmission(0x44);
  Wire.write(0xfd);
  return Wire.endTransmission() == 0;
}
bool sht4x_load() {
  Wire.begin();
  if (!sht4x_cmd(0xfd)) return false;
  delay(10);
  Wire.requestFrom(0x44, 6);
  for (uint8_t i = 0; i < 6; i++) {
    temp_data[i] = Wire.read();
  }
  return (crc8(temp_data, 2) == temp_data[2]) && (crc8(&temp_data[3], 2) == temp_data[5]);
}
float sht4x_temp() {
  if (crc8(temp_data, 2) != temp_data[2]) wendu = -999.0;
  else
    wendu = 175.0 * (((uint16_t) temp_data[0] << 8) + temp_data[1]) / 65536.0 - 45.0;
  return wendu;
}
float sht4x_rh() {
  if (crc8(&temp_data[3], 2) != temp_data[5]) shidu = 101.0;
  else
    shidu = 125.0 * (((uint16_t) temp_data[3] << 8) + temp_data[4]) / 65536.0 - 6.0;
  return shidu;
}
#endif //__SHT4X_H__
