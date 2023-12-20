#ifndef KBD_GUARD
#define KBD_GUARD

#include "types.h"
#include "lib.h"
#include "i8259.h"


#define KBD_DATA 0x60
#define KBD_COMMAND 0x64

#define KBD_IRQ_NUM 0x01

//__attribute__ ((interrupt))
extern void keyboard_init(void);
extern void keyboard_handler(void);
uint8_t key_to_ascii(uint8_t keycode);

#endif
