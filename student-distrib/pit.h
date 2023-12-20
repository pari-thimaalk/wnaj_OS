#pragma once

#include "types.h"
#include "lib.h"
#include "i8259.h"
#include "term.h"
#include "filesystem.h"
#include "syscall_functions.h"

#define BASE_FREQ 1193182
#define INTERRUPT_FREQ 100  //generates an interrupt every 10 milliseconds

//channel 0 is tied to IRQ0, channel 2 is tied to system speaker
//information taken from http://www.osdever.net/bkerndev/Docs/pit.htm
#define BYTE_CMD_REG        0x34    // channel 0, mode 2, low and high bytes, BCD 0
#define CHANNEL_0_PORT      0x40    // data register
#define CHANNEL_2_PORT      0x42    // data register
#define MODE_CMD_REG        0x43    // command register
#define PIT_IRQ                0

void pit_init();
void pit_handler();
