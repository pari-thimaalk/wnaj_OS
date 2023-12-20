#include "e_handler.h"
#include "ass_link_handler.h"
#include "syscall_functions.h"
#include "term.h"

void report_error(char* e_msg){
    //prevent further interrupts from executing
    cli();
    //clears screen, print error then freeze in a while loop
    //clear();
    PRINTLN("---EXCEPTION OCCURED-----\n");
    terminal_write(100,e_msg,strlen(e_msg));
    PRINTLN("--------BAD VIBES--------\n");
    // printf("---EXCEPTION OCCURED-----\n");
    // printf("  error: %s\n",e_msg);
    // printf("--------BAD VIBES--------\n");
    syscall_halt((uint8_t)256);
    //while(1);
    //add common assembly linkage after this point
    //might want to add extra parameter with intr vec number
    //if we want to push its 2's complement onto the stack
}

void divide_error(){
    report_error("divide error\n");
}
void debug_exception(){
    report_error("debug exception error\n");
}
void nmi_exception(){
    report_error("nmi exception\n");
}
void breakpoint_exception(){
    report_error("break point exception\n");
}
void overflow_exception(){
    report_error("overflow exception\n");
}
void oob_exceeeded(){
    report_error("out of bounds exception\n");
}
void invalid_opcode(){
    report_error("invalid opcode exception\n");
}
void device_unavailable(){
    report_error("device unavailable exception\n");
}
void double_fault(){
    report_error("double fault exception\n");
}
void segment_overrun(){
    report_error("segment overrun exception\n");
}
void invalid_tss(){
    report_error("invalid tss exception\n");
}
void segment_not_present(){
    report_error("segment not present exception\n");
}
void stack_segment_fault(){
    report_error("stack segment fault exception\n");
}
void general_protection_fault(){
    report_error("general protection fault exception\n");
}
void page_fault(){
    report_error("page fault exception\n");
}
void reserved15(){
    report_error("reserved 15 exception\n");
}
void floating_pt_exception(){
    report_error("floating point exception\n");
}
void alignment_check(){
    report_error("alignment check exception\n");
}
void machine_check(){
    report_error("machine check exception\n");
}
void simd_floating_pt_exception(){
    report_error("floating point exception\n");
}
void virtualization_exception(){
    report_error("virtualization exception\n");
}

