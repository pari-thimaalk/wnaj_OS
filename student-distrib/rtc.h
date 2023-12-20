#include "lib.h"
#include "i8259.h"

int rtc_flag;
#define RTC_IRQ_NUM     8

//IO Ports
#define INDEX_PORT   0x70
#define CMOS_PORT    0x71
// found on osdev
#define RTC_REG_A     0xA
#define RTC_REG_B     0xB
#define RTC_REG_C     0xC
//nmid stands for nmi disabled
#define RTC_A_NMID   0x8A
#define RTC_B_NMID   0x8B
#define RTC_C_NMID   0x8C
//extra values
#define DV_RS       0x2F
#define UIP_MASK    0x80
#define B_MASK      0x4F
#define RS_MASK     0xF0

int delay;
//initialize the rtc to send interrupts
extern void rtc_init();
extern void rtc_handler();

//because of common interface, the following functions have the same signature as
//as all the other open close read write files
int32_t rtc_open(const uint8_t* filename);
int32_t rtc_close(int32_t fd);
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);

int get_randomizer();
