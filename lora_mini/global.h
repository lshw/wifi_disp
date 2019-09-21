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
  pinMode(2,INPUT); //DIO0
  pinMode(3,INPUT); //DIO1
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
  pinMode(A0,INPUT); //DIO2
  pinMode(A1, INPUT); //POWER ADC
  pinMode(A2, INPUT); //POWER ADC EN
  out_low(A3);
  out_low(A4);
  out_low(A5);
}
float bat(){
pinMode(A2,OUTPUT);
digitalWrite(A2,LOW);
uint16_t volatile val;
val=analogRead(A1);
val=analogRead(A1);
pinMode(A2,INPUT);
return (float) 3.3*(val/1023.0)*2;
}
#endif
