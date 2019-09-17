#include <SPI.h>
#include <LoRa.h>
#define NSS_PIN    	2
#define RESET_PIN 	1
LoRa lora;
uint8_t len;
uint8_t rxBuf[256];
uint32_t send_delay = 0;
uint16_t lora_count = 0;

void lora_send_loop() {
  if (millis() - send_delay < 200) return;
  send_delay = millis();
  lora_count++;
  sprintf(disp_buf, "S%04d", lora_count % 10000);
  disp(disp_buf);
  lora.sendPackage((uint8_t *)disp_buf, 5); // sending data
  lora.idle();    // turn to standby mode
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
    rxBuf[0] = 'L';
    disp((char *)rxBuf);
    lora.rxInit();

    Serial.print("with RSSI ");
    Serial.println(lora.readRSSI());
  }
}
bool lora_state = false;
void lora_init() {
  if (lora_state) return;
  lora_state = lora.init(2,1);
  lora.sleep();    // turn to standby mode
    lora.setFrequency(434E6); //434Mhz
    lora.setRFpara(LR_BW_7p8k,LR_CODINGRATE_2,12,LR_PAYLOAD_CRC_ON); //BW带宽,CR编码率,SF扩频因子，CRC
    // preamble length is 6~65535
    lora.setPreambleLen(12);
    // mode LR_IMPLICIT_HEADER_MODE or LR_EXPLICIT_HEADER_MODE
    lora.setHeaderMode(LR_EXPLICIT_HEADER_MODE);
}
