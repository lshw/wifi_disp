/*从power.h 挖出来的动态调频函数,power.h不支持 __AVR_ATmega328PB__ */
#define _8Mhz 0
#define _4Mhz 1
#define _2Mhz 2
#define _1Mhz 3
#define _500Khz 4
#define _250Khz 5
#define _125Khz 6
#define _62.5Khz 7
#define _31.25Khz 8

#ifndef clock_prescale_get()
//动态调整频率 0->8mhz,1->4mhz,2->2mhz,3->1mhz,4->500khz,5>250khz,6->125khz,7->62.5khz,8->31.25khz

#define clock_prescale_get()  (uint8_t)(CLKPR & (uint8_t)((1<<CLKPS0)|(1<<CLKPS1)|(1<<CLKPS2)|(1<<CLKPS3)))
void clock_prescale_set(uint8_t __x) // 0->8mhz,1->4mhz,2->2mhz,3->1mhz,4->500khz,5>250khz,6->125khz,7->62.5khz,8->31.25khz
{
  uint8_t __tmp = _BV(CLKPCE);
  __asm__ __volatile__ (
    "in __tmp_reg__,__SREG__" "\n\t"
    "cli" "\n\t"
    "sts %1, %0" "\n\t"
    "sts %1, %2" "\n\t"
    "out __SREG__, __tmp_reg__"
    : /* no outputs */
    : "d" (__tmp),
    "M" (_SFR_MEM_ADDR(CLKPR)),
    "d" (__x)
    : "r0");
}
#endif
