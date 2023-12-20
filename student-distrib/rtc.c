#include "rtc.h"
#include "types.h"
#include "term.h"
//variables used in rtc virtualization
static int rtc_freq[NUM_TTYS] = {0,0,0};
static int rtc_counter[NUM_TTYS] = {0,0,0};
static int rtc_count;

/* 
 * rtc_init
 *  DESCRIPTION: Initializes the rtc onto the pic
 *  INPUTS: None
 *  OUTPUTS: None
 *  RETURN VALUE: None
 *  SIDE EFFECTS: will set up rtc to send periodic irqs at a rate of 512Hz
 */
void rtc_init(){
    //turn on periodic interrupts, osdev https://wiki.osdev.org/RTC
    //will turn on irq with default 1024Hz rate
    //reversed the order of outport bc we are using gas syntax, osdev uses intel syntax
    cli();
    //short and simple
    outb(RTC_B_NMID, INDEX_PORT);
    char prev = inb(CMOS_PORT);
    outb(RTC_B_NMID, INDEX_PORT);
    outb(prev | 0x40, CMOS_PORT);

    //change to 512 to reduce overhead, it was also mentioned in a piazza post that it is allowed
    //https://piazza.com/class/llgsmjjyd5d1hd/post/651
    outb(RTC_A_NMID,INDEX_PORT);    // set index to register A, disable NMI
    char prev1=inb(CMOS_PORT);  // get initial value of register A
    outb(RTC_A_NMID,INDEX_PORT);    // reset index to A
    outb((prev1 & 0xF0) | 0x07, CMOS_PORT); //0x07 sets it to 512Hz, calculation can be found on osdev
    rtc_count = 0;
    sti();
    enable_irq(RTC_IRQ_NUM);
    delay = 0;
}

/* 
 * rtc_open
 *  DESCRIPTION: reset freq to 2hz
 *  INPUTS: NULL
 *  OUTPUTS: 0
 */
int32_t rtc_open(const uint8_t* filename)
{
    //set frequency of current terminal to 2hz
    rtc_freq[scheduled_term] = 2;
    return 0;
}
/* 
 * rtc_close
 *  DESCRIPTION: do nothing
 *  INPUTS: None
 *  OUTPUTS: None
 */
int32_t rtc_close(int32_t fd)
{
    //doing this to be safe
    //rtc_counter[scheduled_term] = 0; rtc_freq[scheduled_term] = 0;
    return 0;
}

/* 
 * rtc_read
 *  DESCRIPTION: waits for interrupt, time taken depends on what frequenct was set to
 *  INPUTS: None
 *  OUTPUTS: None
 */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes)
{
    //virtualization!!
    rtc_flag = 1;
    rtc_counter[scheduled_term] = 512/rtc_freq[scheduled_term];
    sti();
    while(rtc_counter[scheduled_term]);
    //keep booing. this is the biggest moment in your year seeing me
    return 0;
}
/* 
 * rtc_write
 *  DESCRIPTION: clock rate change
 *  INPUTS: buf contains the frequency that we wish to set it to
 *  RETURN VALUE: 0 (successful change), -1 (freq provided was not a power of 2, failure)
 */
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes)
{
    //more virtualization!!
    //int rate;
    int32_t freq = *(int32_t*)buf;//get frequency from buffer
    // outb(RTC_A_NMID, INDEX_PORT);
	// unsigned char a = inb(CMOS_PORT);
    if((freq & (freq - 1)) != 0){return -1;};

    rtc_freq[scheduled_term] = freq;

    // int two_power = -1;
    // while(freq){freq /= 2; two_power++;}
    // rate = 16 - two_power;
    
    // outb(RTC_A_NMID, INDEX_PORT);
	// outb((a & RS_MASK) | 0x06, CMOS_PORT);
    return 0;
}

int randomizer;

unsigned read_cmos(int reg) {outb(reg, INDEX_PORT); return inb(CMOS_PORT);}
void print_time(){
    int sec = read_cmos(0x00);
    int min = read_cmos(0x02);
    int hr = read_cmos(0x04);
    int day = read_cmos(0x07);
    int mon = read_cmos(0x08);
    int yr = read_cmos(0x09);

    hr = (hr/16) * 10 + (hr & 0x0f);
    min = (min/16) * 10 + (min & 0x0f);
    sec = (sec/16) * 10 + (sec & 0x0f);
    yr = (yr/16) * 10 + (yr & 0x0f);
    mon = (mon/16) * 10 + (mon & 0x0f);
    day = (day/16) * 10 + (day & 0x0f);

    randomizer = ((sec * 20 + min * 10+hr)/(day)) % 10;

    int month_day_lookup[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (yr % 4 == 0) month_day_lookup[1] = 29;
    if (yr % 100 == 0) month_day_lookup[1] = 28;
    if (yr % 400 == 0) month_day_lookup[1] = 29;

    if (hr - 6 < 0) {hr = hr + 18; day = day - 1;} else {hr = hr - 6;}
    if (day < 0) {day = day + month_day_lookup[mon - 1]; mon = mon - 1;}
    if (mon < 0) {mon = mon + 12; yr--;} 
    //put directly into video memory
    int8_t sec_[3]; if(sec > 9){itoa(sec, sec_, 10);}else{itoa(sec, &sec_[1],10);sec_[0] = '0';}
    int8_t mins_[3]; if(min > 9){itoa(min, mins_, 10);}else{itoa(min,&mins_[1],10);mins_[0] = '0';}
    int8_t hour_[3]; if(hr > 9){itoa(hr, hour_, 10);}else{itoa(hr,&hour_[1],10);hour_[0] = '0';}
    int8_t year_[3]; if(yr > 9){itoa(yr, year_, 10);}else{itoa(yr,&year_[1],10);year_[0] = '0';}
    int8_t month_[3]; if(mon > 9){itoa(mon, month_, 10);}else{itoa(mon,&month_[1],10);month_[0] = '0';}
    int8_t day_[3]; if(day > 9){itoa(day, day_, 10);}else{itoa(day,&day_[1],10);day_[0] = '0';}

    *(uint8_t*)(0xB7000 + (24*80 + 78) * 2) = sec_[0];
    *(uint8_t*)(0xB7000 + (24*80 + 79) * 2) = sec_[1];
    *(uint8_t*)(0xB7000 + (24*80 + 77) * 2) = ':';
    *(uint8_t*)(0xB7000 + (24*80 + 75) * 2) = mins_[0];
    *(uint8_t*)(0xB7000 + (24*80 + 76) * 2) = mins_[1];
    *(uint8_t*)(0xB7000 + (24*80 + 74) * 2) = ':';
    *(uint8_t*)(0xB7000 + (24*80 + 72) * 2) = hour_[0];
    *(uint8_t*)(0xB7000 + (24*80 + 73) * 2) = hour_[1];
    *(uint8_t*)(0xB7000 + (24*80 + 71) * 2) = ' ';
    *(uint8_t*)(0xB7000 + (24*80 + 69) * 2) = year_[0];
    *(uint8_t*)(0xB7000 + (24*80 + 70) * 2) = year_[1];
    *(uint8_t*)(0xB7000 + (24*80 + 68) * 2) = '/';
    *(uint8_t*)(0xB7000 + (24*80 + 66) * 2) = month_[0];
    *(uint8_t*)(0xB7000 + (24*80 + 67) * 2) = month_[1];
    *(uint8_t*)(0xB7000 + (24*80 + 65) * 2) = '/';
    *(uint8_t*)(0xB7000 + (24*80 + 63) * 2) = day_[0];
    *(uint8_t*)(0xB7000 + (24*80 + 64) * 2) = day_[1];
}

int get_randomizer() {
    return randomizer;
}
/* 
 * rtc_handler
 *  DESCRIPTION: Handles interrupts sent from the rtc
 *  INPUTS: None
 *  OUTPUTS: None
 *  RETURN VALUE: None
 *  SIDE EFFECTS: decrements counters if they are running
 */
void rtc_handler(){
    cli();
    int j,k;
    //test_interrupts();
    rtc_flag = 0;
    for(j = 0; j < NUM_TTYS; j++){
        if(rtc_counter[j] != 0)rtc_counter[j] --;
    }
    //we need to read register c after an irq8, else
    //the interrupt will not happen again, osdev rtc
    outb(RTC_REG_C, INDEX_PORT);	// select register C
    inb(CMOS_PORT);                      // just throw away contents
    //let pic know interrupt handling is finished
    send_eoi(RTC_IRQ_NUM);
    rtc_count++;
    if(rtc_count % 512 == 0){print_time(); rtc_count = 0;}
    if(delay > 0){delay --;}
    if(delay == 1){for (k = 10; k < 63; k++) *(uint8_t *)(0xb7000 + ((NUM_COLS * 24 + k) << 1)) = ' ';}
    sti();
}




