#include <SPI.h>
#include "sx1278.h"

LoRa lora;
uint8_t rxBuf[250] = {0}, rxLen = 0;
uint32_t send_delay = 0;
uint16_t lora_count = 0;
extern char disp_buf[22];
void lora_sleep() {
  lora.sleep();
}
void lora_send_wendu() {
  String msg;
  msg = val_str();
  lora.setPayloadLength(msg.length());
  lora.sendPackage((uint8_t *)msg.c_str(), msg.length()); // sending data
  lora.idle();    // turn to standby mode
}
void lora_send_loop() {
  if (millis() < send_delay) return;
  send_delay = millis() + 200;
  lora_count++;
  snprintf_P(disp_buf, sizeof(disp_buf), PSTR("S%4d"), lora_count % 10000);
  disp(disp_buf);
  lora.sendPackage((uint8_t *)disp_buf, 5); // sending data
  lora.idle();    // turn to standby mode
  yield();
}
uint8_t lora_rxtx = 0; //1:rx 2:tx
void lora_receive_proc4() {
  if (lora_rxtx != 1) {
    lora.rxInit();
    lora_rxtx = 1;
  }
  if (lora.waitIrq()) {
    lora.clearIRQFlags();
    rxLen = lora.receivePackage(rxBuf);
    if (rxLen < sizeof(rxBuf))
      rxBuf[rxLen] = 0;
    Serial.printf_P(PSTR("%ld lora recv:[%s]\r\n"), millis(), rxBuf);
    Serial.write(rxBuf, rxLen);
    Serial.println();
    snprintf_P(disp_buf, sizeof(disp_buf), PSTR("P4.%d"), rxLen);
    disp(disp_buf);
  }
  yield();
}
void lora_receive_loop() {
  if (lora_rxtx != 1) {
    lora.rxInit();
    lora_rxtx = 1;
  }
  if (lora.waitIrq()) {
    lora.clearIRQFlags();
    rxLen = lora.receivePackage(rxBuf);
    Serial.write(rxBuf, rxLen);
    Serial.println();
    uint8_t rssi = 137 - lora.readRSSI(1);
    snprintf_P(disp_buf, sizeof(disp_buf), PSTR("%3d.%c%c"), rssi, rxBuf[3], rxBuf[4]);
    disp(disp_buf);
    Serial.print(F("with RSSI "));
    Serial.print(rssi);
    Serial.println(F("dBm"));
  }
  yield();
}
bool lora_init() {
  if (lora_version != 255 && lora_version != 0) return true;
  lora.init(2); //cs=2
  if (lora_version == 255 || lora_version == 0) {
    if (nvram.have_lora > -5) {
      nvram.change = 1;
      nvram.have_lora --;
    }
    return false;
  }
  if (nvram.have_lora < 1) {
    nvram.have_lora = 1;
    nvram.change = 1;
  }
  lora.setFrequency(434500000); //434Mhz
  lora.setRFpara(nvram.bw, nvram.cr, nvram.sf, LR_PAYLOAD_CRC_ON); //BW带宽,CR编码率,SF扩频因子，CRC
  // preamble length is 6~65535
  lora.setPreambleLen(12); //前导12
  lora.setPayloadLength(10);//数据长度10
  lora.setTxPower(15); //default 最大发送20db
  // mode LR_IMPLICIT_HEADER_MODE or LR_EXPLICIT_HEADER_MODE
  lora.setHeaderMode(LR_EXPLICIT_HEADER_MODE);//不要header
  lora.idle();    // turn to standby mode
  return true;
}
