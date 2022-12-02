#include <SPI.h>
#define NSS_PIN    	2
#define RESET_PIN 	1
#include "sx1278.h"
LoRa lora;
uint8_t len;
uint8_t rxBuf[256];
uint32_t send_delay = 0;
uint16_t lora_count = 0;
void lora_send_loop() {
  if (millis() - send_delay < 200) return;
  system_soft_wdt_feed ();
  send_delay = millis();
  lora_count++;
  snprintf_P(disp_buf, sizeof(disp_buf), PSTR("S%4d"), lora_count % 10000);
  disp(disp_buf);
  lora.sendPackage((uint8_t *)disp_buf, 5); // sending data
  lora.idle();    // turn to standby mode
  yield();
}
uint8_t lora_rxtx = 0; //1:rx 2:tx
void lora_receive_loop() {
  if (lora_rxtx != 1) {
    lora.rxInit();
    lora_rxtx = 1;
  }
  if (lora.waitIrq()) {
    lora.clearIRQFlags();
    len = lora.receivePackage(rxBuf);
    Serial.write(rxBuf, len);
    Serial.println();
    uint8_t rssi = 137 - lora.readRSSI(1);
    snprintf_P(disp_buf, sizeof(disp_buf), PSTR("%3d.%c%c"), rssi, rxBuf[3], rxBuf[4]);
    disp(disp_buf);
    Serial.print(F("with RSSI "));
    Serial.print(rssi);
    Serial.println(F("dBm"));
    system_soft_wdt_feed ();
  }
  yield();
}
bool lora_init() {
  if (lora_version != 255) return true;
  lora.init(2, 1);
  if (lora_version == 255) {
    if (nvram.have_lora > -5) {
      nvram.change = 1;
      nvram.have_lora --;
    }
    return false;
  }
  if (nvram.have_dht > -5) {
    nvram.have_dht = -5;
    nvram.change = 1;
  }
  if (nvram.have_lora < 1) {
    nvram.have_lora = 1;
    nvram.change = 1;
  }
  lora.idle();    // turn to standby mode
  lora.setFrequency(434500000); //434Mhz
  lora.setRFpara(LR_BW_250k, LR_CODINGRATE_2, 12, LR_PAYLOAD_CRC_ON); //BW带宽,CR编码率,SF扩频因子，CRC
  // preamble length is 6~65535
  lora.setPreambleLen(12); //前导12
  //lora.setPayloadLength(10);//数据长度10
  //lora.setTxPower(15); //default 最大发送20db
  // mode LR_IMPLICIT_HEADER_MODE or LR_EXPLICIT_HEADER_MODE
  lora.setHeaderMode(LR_EXPLICIT_HEADER_MODE);//不要header
  return true;
}
