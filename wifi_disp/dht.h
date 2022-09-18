#ifndef __DHT_H__
#define __DHT_H__

#include <dhtnew.h>
#define DHT_VCC 13
#define DHTPIN 14
#define DHTTYPE DHT22

DHTNEW mySensor(DHTPIN);
uint32_t next_load;
extern uint8_t ds_pin;
extern uint8_t lora_version;
float shidu = -999.0;
float wendu = -900.0;
bool test();
void system_soft_wdt_feed ();
void dht_load() { //阻塞测试
  if (lora_version != 255) return; //存在lora模块
  if (ds_pin != 0) return; //v1硬件不带湿度
  while (mySensor.read() != DHTLIB_OK) {
    if (millis() > 2100) return;
    system_soft_wdt_feed();
    if (test()) break;
  }
  shidu = mySensor.getHumidity();
  wendu = mySensor.getTemperature();
  Serial.println("型号:" + String(mySensor.getType()) + " 湿度:" + String(shidu) + " 温度" + String(wendu));
}
bool dht_loop()
{ //不阻塞测试
  if (lora_version != 255) return false; //存在lora模块
  if (ds_pin != 0) return false; //v1硬件不带湿度
  if (wendu < -300.0) {
    if (mySensor.read() == DHTLIB_OK) {
      shidu = mySensor.getHumidity();
      wendu = mySensor.getTemperature();
      Serial.println("型号:" + String(mySensor.getType()) + "湿度:" + String(shidu) + " 温度" + String(wendu));
      return true;
    }
  }
  return false;
}
bool dht_setup()
{
  if (lora_version != 255) return false ; //存在lora模块
  if (ds_pin != 0) return false; //v1硬件不带湿度
  pinMode(DHT_VCC, OUTPUT); //gpio13 电源
  digitalWrite(DHT_VCC, HIGH);
  next_load = millis() + 2000;
  Serial.println("STAT\tHUMI\tTEMP\tTIME\tTYPE");
  if (!test())
    return  test();
  return true;
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
  if (lora_version != 255) return; //存在lora模块
  if (ds_pin != 0) return ; //v1硬件不带湿度
  digitalWrite(DHT_VCC, LOW);
  pinMode(DHTPIN, OUTPUT);
  digitalWrite(DHTPIN, LOW);
}


bool test()
{
  if (lora_version != 255) return true; //存在lora模块
  if (ds_pin != 0) return true; //v1硬件不带湿度
  // READ DATA
  uint32_t start = micros();
  int chk = mySensor.read();
  uint32_t stop = micros();
  //  Serial.print("vcc="); Serial.print(digitalRead(DHT_VCC));
  //  Serial.print(",data="); Serial.println(digitalRead(DHTPIN));

  switch (chk)
  {
    case DHTLIB_OK:
      Serial.print("OK,\t");
      break;
    case DHTLIB_ERROR_CHECKSUM:
      Serial.print("Checksum error,\t");
      break;
    case DHTLIB_ERROR_TIMEOUT_A:
      Serial.print("Time out A error,\t");
      break;
    case DHTLIB_ERROR_TIMEOUT_B:
      Serial.print("Time out B error,\t");
      break;
    case DHTLIB_ERROR_TIMEOUT_C:
      Serial.print("Time out C error,\t");
      break;
    case DHTLIB_ERROR_TIMEOUT_D:
      Serial.print("Time out D error,\t");
      break;
    default:
      Serial.print("Unknown error,\t");
      break;
  }
  // DISPLAY DATA
  Serial.print(mySensor.getHumidity(), 1);
  Serial.print(",\t");
  Serial.print(mySensor.getTemperature(), 1);
  Serial.print(",\t");
  uint32_t duration = stop - start;
  Serial.print(duration);
  Serial.print(",\t");
  Serial.println(mySensor.getType());

  if (chk == DHTLIB_OK) {
    shidu = mySensor.getHumidity();
    wendu = mySensor.getTemperature();
    Serial.println("型号:" + String(mySensor.getType()) + "湿度:" + String(shidu) + " 温度" + String(wendu));
    return true;
  }
  delay(500);
  return false;
}
#endif
