#include "kbd.h"
#include "rtc.h"


//linkage functions for rtc,keyboard, and system calls, check .S file for implementation
//these functions will just call their respective handler functions to handle the interrupts
extern void rtc_linkage();
extern void keyboard_linkage();
extern void syscall_linkage();
extern void mouse_linkage();
extern void pit_linkage();
