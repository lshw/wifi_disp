#ifndef __NVRAM__
#define __NVRAM__

#define NVRAM7_CHARGE 0b1
#define NVRAM7_URL    0b10
#define NVRAM7_UPDATE 0b1000
#define HAVE_DHT 0b1
#define HAVE_LORA 0b10

struct {
  uint8_t proc;
  uint8_t nvram7;
  uint32_t rate;
  uint8_t comset;
  uint8_t change;
  uint32_t boot_count;
  uint32_t crc32;
} nvram;

uint32_t calculateCRC32(const uint8_t *data, size_t length);
void load_nvram() {
  ESP.rtcUserMemoryRead(0, (uint32_t*) &nvram, sizeof(nvram));
  if (nvram.crc32 != calculateCRC32((uint8_t*) &nvram, sizeof(nvram) - sizeof(nvram.crc32))) {
    memset(&nvram, 0, sizeof(nvram));
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
