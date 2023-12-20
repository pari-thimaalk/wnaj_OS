#include "lib.h"
extern void report_error(char* e_msg);
//exceptions 0x00-0x13
extern void divide_error();
extern void debug_exception();
extern void nmi_exception();
extern void breakpoint_exception();
extern void overflow_exception();
extern void oob_exceeeded();
extern void invalid_opcode();
extern void device_unavailable();
extern void double_fault();
extern void segment_overrun();
extern void invalid_tss();
extern void segment_not_present();
extern void stack_segment_fault();
extern void general_protection_fault();
extern void page_fault();
extern void reserved15();
extern void floating_pt_exception();
extern void alignment_check();
extern void machine_check();
extern void simd_floating_pt_exception();

