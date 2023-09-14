#ifndef __NVRAM__
#define __NVRAM__
#include "sx1278_reg.h"
#define NVRAM7_CHARGE 0b1
#define NVRAM7_URL    0b10
struct {
  uint8_t proc;
  uint8_t change;
  int8_t  have_dht;
  int8_t  have_lora; //失败一次就减1，减到-5,  设置ota模式， 会清0， 上同
  uint32_t nvram7;
  uint8_t ch;
  uint8_t bw;
  uint8_t cr;
  uint8_t sf;
  uint32_t lora_hz;
  uint16_t proc3_port;
  char proc3_host[34];
  uint16_t proc3_sec; //多少秒测一次
  int8_t pcb_ver;
  int8_t ds18b20_pin;
  uint32_t boot_count;
  uint8_t old_proc;
  uint8_t baoliu[3];
  uint32_t crc32;
}  nvram;

uint32_t calculateCRC32(const uint8_t *data, size_t length);
void save_nvram();
void load_nvram() {
  File fp;
  ESP.rtcUserMemoryRead(0, (uint32_t*) &nvram, sizeof(nvram));
  if (nvram.crc32 != calculateCRC32((uint8_t*) &nvram, sizeof(nvram) - sizeof(nvram.crc32))) {
    if (SPIFFS.begin()) {
      if (SPIFFS.exists("/nvram.bin")) {
        fp = SPIFFS.open("/nvram.bin", "r");
        fp.read((uint8_t *) &nvram, sizeof(nvram));
        fp.close();
        nvram.boot_count = 0;
        nvram.have_lora = 0;
        nvram.ds18b20_pin = -1; //需要自检
        nvram.have_dht = -1; //需要自检
        nvram.pcb_ver = -1;  //需要自检
        nvram.change = 1;
        save_nvram();
      }
    }
  }
  if (nvram.crc32 != calculateCRC32((uint8_t*) &nvram, sizeof(nvram) - sizeof(nvram.crc32))) {
    memset(&nvram, 0, sizeof(nvram));
    nvram.ch = 6;
    nvram.bw = LR_BW_125k;
    nvram.cr = LR_CODINGRATE_2;
    nvram.sf = LR_SPREADING_FACTOR_12;
    nvram.lora_hz = 434500000L;
    nvram.proc3_sec = 20;
    strcpy(nvram.proc3_host, "192.168.2.4");
    nvram.proc3_port = 8888;
    nvram.ds18b20_pin = -1; //需要自检
    nvram.have_dht = -1; //需要自检
    nvram.pcb_ver = -1;  //需要自检
    nvram.change = 1;
    save_nvram();
  } else if (nvram.ch > 0 && nvram.ch <= 14) {
    Serial.printf_P(PSTR("\r\nwifi channel=%d, proc=%d\r\n"), nvram.ch, nvram.proc);
    WRITE_PERI_REG(0x600011f4, 1 << 16 | nvram.ch);
  }
}
void save_nvram() {
  if (nvram.change == 0) return;
  nvram.change = 0;
  nvram.crc32 = calculateCRC32((uint8_t*) &nvram, sizeof(nvram) - sizeof(nvram.crc32));
  ESP.rtcUserMemoryWrite(0, (uint32_t*) &nvram, sizeof(nvram));
}

uint32_t calculateCRC32(const uint8_t *data, size_t length) {
  uint32_t crc = 0xffffffff;
  while (length--) {
    uint8_t c = *data++;
    for (uint32_t i = 0x80; i > 0; i >>= 1) {
      bool bit = crc & 0x80000000;
      if (c & i) {
        bit = !bit;
      }
      crc <<= 1;
      if (bit) {
        crc ^= 0x04c11db7;
      }
    }
  }
  return crc;
}
#endif //__NVRAM__
