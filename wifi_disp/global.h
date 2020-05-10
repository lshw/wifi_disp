#ifndef __GLOBAL_H__
#define __GLOBAL_H__
#include "config.h"
#include "ht16c21.h"
extern uint8_t ds_pin ;
extern bool power_in ;
extern char ram_buf[10];
void send_ram();
float get_batt();
float v;
bool power_off = false;
void poweroff(uint32_t sec) {
  get_batt();
#if DHT_HAVE
  void dht_end();
#endif
  if (ds_pin == 0) Serial.println("V2.0");
  else
    Serial.println("V1.0");
  if (power_in) Serial.println("有外接电源");
  else Serial.println("无外接电源");
  Serial.flush();
  if (ram_buf[7] & 1) {
    if (v > 4.20) {
      Serial.println("v=" + String(v) + ",停止充电");
      ram_buf[7] &= ~1;
      send_ram();
    }
  }
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  if (power_in && (ram_buf[7] & 1)) { //如果外面接了电， 就进入LIGHT_SLEEP模式 电流0.8ma， 保持充电
    sec = sec / 2;
    Serial.print("休眠");
    if (sec > 60) {
      Serial.print(sec / 60);
      Serial.print("分钟");
    }
    if ((sec % 60) != 0) {
      Serial.print(sec % 60);
      Serial.print("秒");
    }
    Serial.println("\r\n充电中");
    Serial.flush();
    wdt_disable();
    uint8_t no_power_in = 0;
    for (uint32_t i = 0; i < sec; i++) {
      system_soft_wdt_feed ();
      delay(1000); //空闲时进入LIGHT_SLEEP_T模式
      power_in = i % 2;
      send_ram();
      get_batt();
      if (ds_pin != 0) {
        pinMode(13, OUTPUT); //v1.0充电控制
        digitalWrite(13, LOW); //1.0硬件
      } else {
        Serial.end();
        pinMode(1, OUTPUT);
        digitalWrite(1, HIGH); //2.0硬件
      }
      if (!power_in) {
        no_power_in++;
      } else {
        if (no_power_in > 0) no_power_in--;
      }
      if (no_power_in > 5) {
        sec = sec + sec - i; //连续6次测试电源断开
        send_ram();
        break;
      }
    }
  }
  if (ds_pin == 0) {
    Serial.begin(115200);
    Serial.println();
  }
  if ((ram_buf[7] & 1) && power_in)
    Serial.println("充电结束");
  Serial.print("关机");
  if (sec > 0) {
    if (sec > 60) {
      Serial.print(sec / 60);
      Serial.print("分钟");
    }
    if ((sec % 60) != 0) {
      Serial.print(sec % 60);
      Serial.print("秒");
    }
    Serial.println("\r\nbye!");
  }
  uint64_t sec0 = sec * 1000000;
  Serial.flush();
  if (ds_pin != 0) {
    pinMode(13, OUTPUT);
    digitalWrite(13, HIGH); //v1.0硬件
  } else {
    Serial.println("关闭充电");
    Serial.flush();
    Serial.end();
    pinMode(1, OUTPUT);
    digitalWrite(1, LOW);
  }
  wdt_disable();
  system_deep_sleep_set_option(4);
  digitalWrite(LED_BUILTIN, LOW);
  if (sec == 0) ht16c21_cmd(0x84, 0x2); //lcd off
  ESP.deepSleep(sec0, WAKE_RF_DEFAULT);
  power_off = true;
}
float get_batt0() {//锂电池电压
  uint32_t dat = analogRead(A0);
  dat = analogRead(A0)
        + analogRead(A0)
        + analogRead(A0)
        + analogRead(A0)
        + analogRead(A0)
        + analogRead(A0)
        + analogRead(A0)
        + analogRead(A0);

  if (ds_pin != 0) //V1.0硬件分压电阻 499k 97.6k
    v = (float) dat / 8 * (499 + 97.6) / 97.6 / 1023 ;
  else    //V2.0硬件 分压电阻 470k/100k
  v = (float) dat / 8 * (470.0 + 100.0) / 100.0 / 1023 ;
  return v;
}
float get_batt() {
  float v0;
  if (ds_pin != 0) { //v1.0硬件
    pinMode(13, OUTPUT);
    digitalWrite(13, HIGH); //不充电
  } else { //v2.0硬件
    Serial.flush();
    Serial.end();
    pinMode(1, OUTPUT);
    digitalWrite(1, LOW); //不充电
  }
  delay(1);
  get_batt0();
  v0 = v;
  if (ds_pin != 0) //v1.0硬件
    digitalWrite(13, LOW); //充电
  else //v2.0硬件
    digitalWrite(1, HIGH); //充电
  delay(1);
  get_batt0();
  if (v > v0) { //有外接电源
    v0 = v;
    if (ds_pin != 0)
      digitalWrite(13, HIGH); //不充电
    else
      digitalWrite(1, LOW); //不充电
    delay(1);
    get_batt0();
    if (v0 > v) {
      v0 = v;
      if (ds_pin != 0)
        digitalWrite(13, LOW); //充电
      else
        digitalWrite(1, HIGH); //充电
      delay(1);
      get_batt0();
      if (v > v0) {
        v0 = v;
        if (ds_pin != 0)
          digitalWrite(13, HIGH); //不充电
        else
          digitalWrite(1, LOW); //不充电
        delay(1);
        get_batt0();
        if (v0 > v) {
          if (!power_in) {
            power_in = true;
            Serial.println("测得电源插入");
          }
        } else power_in = false;
      } else power_in = false;
    } else power_in = false;
  } else power_in = false;

  if (ds_pin != 0)
    digitalWrite(13, HIGH); //不充电
  else
    digitalWrite(1, LOW); //不充电
  delay(1);
  get_batt0();
  if ((ram_buf[7] & 1) == 0) {
    if (v < 3.8) {
      ram_buf[7] |= 1;
      send_ram();
    }
  }
  if (ds_pin == 0) {
    Serial.begin(115200);
  }
  return v;
}

#endif
