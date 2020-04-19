#ifndef __DHT_H__
#define __DHT_H__

#include <dhtnew.h>
#define DHT_VCC 13
#define DHTPIN 14
#define DHTTYPE DHT22

DHTNEW mySensor(DHTPIN);
uint32_t next_load;
extern uint8_t ds_pin;
float shidu = -999.0;
float wendu;
bool test();
void system_soft_wdt_feed ();
void dht_load() { //阻塞测试
  if (ds_pin != 0) return; //v1硬件不带湿度
  while (mySensor.read() != DHTLIB_OK) {
    system_soft_wdt_feed();
    if (test()) break;
  }
  shidu = mySensor.humidity;
  wendu = mySensor.temperature;
  Serial.println("型号:" + String(mySensor.getType()) + " 湿度:" + String(shidu) + " 温度" + String(wendu));
}
bool dht_loop()
{ //不阻塞测试
  if (ds_pin != 0) return false; //v1硬件不带湿度
  if (wendu < -300.0) {
    if (mySensor.read() == DHTLIB_OK) {
      shidu = mySensor.humidity;
      wendu = mySensor.temperature;
      Serial.println("型号:" + String(mySensor.getType()) + "湿度:" + String(shidu) + " 温度" + String(wendu));
      return true;
    }
  }
  return false;
}
void dht_setup()
{
  if (ds_pin != 0) return ; //v1硬件不带湿度
  pinMode(DHT_VCC, OUTPUT); //gpio13 电源
  digitalWrite(DHT_VCC, HIGH);
  next_load = millis() + 2000;
  Serial.println("STAT\tHUMI\tTEMP\tTIME\tTYPE");
  test();
  test();
  test();
  test();
  /*
    Serial.println("\n4. LastRead test");
    mySensor.read();
    for (int i = 0; i < 20; i++)
    {
      if (millis() - mySensor.lastRead() > 1000)
      {
        mySensor.read();
        Serial.println("actual read");
      }
      Serial.print(mySensor.humidity, 1);
      Serial.print(",\t");
      Serial.println(mySensor.temperature, 1);
      delay(250);
    }
  */
}
void dht_end()
{
  if (ds_pin != 0) return ; //v1硬件不带湿度
  digitalWrite(DHT_VCC, LOW);
  pinMode(DHTPIN, OUTPUT);
  digitalWrite(DHTPIN, LOW);
}


bool test()
{
  // READ DATA
  uint32_t start = micros();
  int chk = mySensor.read();
  uint32_t stop = micros();
  Serial.print("vcc="); Serial.print(digitalRead(DHT_VCC));
  Serial.print(",data="); Serial.println(digitalRead(DHTPIN));

  switch (chk)
  {
    case DHTLIB_OK:
      Serial.print("OK,\t");
      break;
    case DHTLIB_ERROR_CHECKSUM:
      Serial.print("Checksum error,\t");
      break;
    case DHTLIB_ERROR_TIMEOUT:
      Serial.print("Time out error,\t");
      break;
    default:
      Serial.print("Unknown error,\t");
      break;
  }
  // DISPLAY DATA
  Serial.print(mySensor.humidity, 1);
  Serial.print(",\t");
  Serial.print(mySensor.temperature, 1);
  Serial.print(",\t");
  uint32_t duration = stop - start;
  Serial.print(duration);
  Serial.print(",\t");
  Serial.println(mySensor.getType());

  if (chk == DHTLIB_OK) {
    shidu = mySensor.humidity;
    wendu = mySensor.temperature;
    Serial.println("型号:" + String(mySensor.getType()) + "湿度:" + String(shidu) + " 温度" + String(wendu));
    return true;
  }
  delay(500);
  return false;
}
#endif
