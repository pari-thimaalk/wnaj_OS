#include "signals.h"
#include "syscall_functions.h"

//helpful paging related macros
#define EIGHT_MB 0x800000
#define EIGHT_KB 0x2000
#define FOUR_MB_PAGE 0x400000

//for div_zero, segfault and interrupt
int32_t kill_action(){
    PRINTLN("Hello!");
    syscall_halt((uint8_t)256);
    return 0;
}

//for alarm and user1
int32_t continue_action(){return 0;}

void signal_init(){
    int i;
    for(i = 0; i < 3; i++){sig_handlers[i] = kill_action;}
    for(i = 3; i < 5; i++){sig_handlers[i] = continue_action;}
}

//called when signal condition is triggered
void send_signal(int signum){
    cli();
    int relevant_pid;
    if(signum == 2){
        relevant_pid = terms[visible_term].pid; //bc sig2 is ctrl+c, a keyboard based interrupt
    } else {relevant_pid = terms[scheduled_term].pid;}

    pcb_t* relevant_pcb = (pcb_t*) (EIGHT_MB - (relevant_pid + 1)*EIGHT_KB);
    //raise the signal of the relevant pcb
    relevant_pcb->pending_sig |= (1 << signum);
    sti();
}

//called when returning from an interrupt, exception or system call
void check_deliver_signal(uint32_t user_esp, uint32_t user_ebp){
    int has_pending = 0;
    int i;  //signum that we want to execute
    for(i = 0; i < 5; i++){
        if((this_pcb->pending_sig >> i) & 0x01){has_pending = 1; break;}
    }
    //if no pending signals, return
    if(!has_pending){return;}

    //mask all other signals
    this_pcb->masked_sig = this_pcb->pending_sig & (0x01 << i);

    
}







