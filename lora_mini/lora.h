#include <SPI.h>
#include <LoRa.h>
#define DIO0 2
#define DIO1 3
#define DIO2 A0
#define NSS_PIN  10
#define RESET_PIN  9
#define LORA_VCC 8
LoRa lora;
uint8_t len;
char buf[256];
uint16_t lora_count = 0;

void lora_send_loop() {
  lora_count++;
  sprintf(buf, "S%04d", lora_count % 10000);
  //  disp(disp_buf);
  lora.sendPackage((uint8_t *)buf, 5); // sending data
  lora.idle();    // turn to standby mode
  //  lora.sleep();
}
uint8_t lora_rxtx = 0; //1:rx 2:tx
void lora_receive_loop() {
  if (lora_rxtx != 1) {
    lora.rxInit();
    lora_rxtx = 1;
  }
  if (lora.waitIrq()) {
    lora.clearIRQFlags();
    len = lora.receivePackage(buf);
    Serial.write(buf, len);
    Serial.println();
    buf[0] = 'L';
    //    disp((char *)rxBuf);
    lora.rxInit();

    Serial.print("with RSSI ");
    uint8_t rssi = 137 - lora.readRSSI(1); //-dBm
    Serial.println(rssi);
  }
}
bool lora_state = false;
void lora_init() {
  //  if (lora_state && digitalRead(LORA_VCC) == LOW) return;
  //  digitalWrite(LORA_VCC, LOW);
  //  delay(1);
  lora_state = lora.init(10, 9);
  lora.sleep();
  lora.setFrequency(434500000); //434.5Mhz
  lora.setRFpara(LR_BW_250k, LR_CODINGRATE_2, 12, LR_PAYLOAD_CRC_ON); //BW带宽,CR编码率,SF扩频因子，CRC
  // preamble length is 6~65535
  lora.setPreambleLen(60);//前导60
  //lora.setPayloadLength(10); //数据长度10
  //lora.setTxPower(15); //default set 20db
  // mode LR_IMPLICIT_HEADER_MODE or LR_EXPLICIT_HEADER_MODE
  lora.setHeaderMode(LR_EXPLICIT_HEADER_MODE);//不要header
}
void lora_off() {
  lora_state = false;
  digitalWrite(LORA_VCC, HIGH);
}
