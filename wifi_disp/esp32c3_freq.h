/*
  参考文档:
  https://www.espressif.com.cn/sites/default/files/documentation/esp32-c3_technical_reference_manual_cn.pdf
  174页
*/
#ifndef __ESP32C3_FREQ_H__
#define __ESP32C3_FREQ_H__
#include <soc/system_reg.h>
void esp32c3_set_freq_mhz(uint8_t sel) {
  if (sel == 0)
    sel = 160;
  if (sel <= 40) {
    REG_SET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_SOC_CLK_SEL, 0);
    REG_SET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_PRE_DIV_CNT, (uint8_t)40 / sel - 1);
  } else {
    REG_SET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_SOC_CLK_SEL, 1);
    REG_SET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_PLL_FREQ_SEL, 1);
    if (sel == 80)
      REG_SET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_CPUPERIOD_SEL, 0);
    else
      REG_SET_FIELD(SYSTEM_SYSCLK_CONF_REG, SYSTEM_CPUPERIOD_SEL, 1);
  }
}

#endif  //__ESP32C3_FREQ_H__
