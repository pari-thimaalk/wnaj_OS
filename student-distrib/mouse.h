#include "lib.h"
#include "i8259.h"

#define MOUSE_IRQ 12
//byte commands
#define ENABLE_AUX_DEV 0xA8 //enable auxillary device command
#define GET_CQ_STATUS 0x20 //get compaq status byte
#define SET_CQ_STATUS 0x60 //set compaq status byte

#define MOUSE_IO_64 0x64
#define MOUSE_IO_60 0x60

//port, register and command numbers taken from https://wiki.osdev.org/Mouse_Input
void mouse_write(uint8_t byte);
void mouse_wait_write();
void mouse_wait_read();
extern void mouse_init();
extern void mouse_handler();
