#ifndef _GLOBAL_H_
#define _GLOBAL_H_
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/boot.h>
#include <avr/wdt.h>
#include "wdt_time.h"
#include "lora.h"
#include "ds1820.h"

//让悬空的GPIO都置低
//低功耗初始化 gpio模式
#define out_low(pin) pinMode(pin,OUTPUT);digitalWrite(pin,LOW);
#define out_high(pin) pinMode(pin,OUTPUT);digitalWrite(pin,HIGH);
#define input_up(pin) pinMode(pin,INPUT_PULLUP);
void set_gpio() {
  out_low(0); //RX
  out_low(1); //TX
  input_up(2); //DIO0
  input_up(3); //DIO1
  out_low(4); //ds_vcc
  out_low(5); //ds1820
  out_low(6);
  out_low(7);
  out_high(8); //lora vcc high=off
  input_up(9); //lora reset
  //out_low(10); //lora cs
  //out_low(11); //MOSI
  input_up(12); //MISO
  out_low(13); //SCK
  input_up(A0); //DIO2
  pinMode(A1, INPUT); //POWER ADC
  input_up(A2); //POWER ADC EN
  out_low(A3);
  out_low(A4);
  out_low(A5);
}
#endif
