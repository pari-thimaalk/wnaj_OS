#include "pit.h"

//helpful paging related macros
#define EIGHT_MB 0x800000
#define EIGHT_KB 0x2000
#define FOUR_MB_PAGE 0x400000


/* 
 *  pit_init
 *  DESCRIPTION: initialises pit
 *  INPUTS: None
 *  OUTPUTS: None
 *  RETURN VALUE: None
 *  SIDE EFFECTS: initializes pit
 */
void pit_init(){
    //short and sweet, taken from osdev http://www.osdever.net/bkerndev/Docs/pit.htm
    //initialised pit to send regular interrupts at Interrupt_Freq Hz
    int divisor = BASE_FREQ/INTERRUPT_FREQ;
    outb(BYTE_CMD_REG,MODE_CMD_REG);
    outb(divisor & 0xFF,CHANNEL_0_PORT);
    outb(divisor >> 8, CHANNEL_0_PORT);
    enable_irq(PIT_IRQ);

}

/* 
 *  pit_handler
 *  DESCRIPTION: contains code for what we should do when pit sends interrupt
 *  INPUTS: none
 *  OUTPUTS: None
 *  RETURN VALUE: fd num
 *  SIDE EFFECTS: performs context switch from one kernel "thread" to another - round robin scheduling algorithm
 *                  sends eoi signal to indicate interrupt has been serviced
 */
void pit_handler(){
    //disregarding the very first iteration, we will save current ebp/esp values
    if(terms[scheduled_term].pid != -1){
        pcb_t* current_pcb = (pcb_t*) (EIGHT_MB - (terms[scheduled_term].pid + 1)*EIGHT_KB);
        asm("movl %%ebp,%0" : "=r"(current_pcb->scheduled_ebp));
        asm("movl %%esp,%0" : "=r"(current_pcb->scheduled_esp));
    }

    //move to next process
    scheduled_term = (scheduled_term + 1) % NUM_TTYS;

    //remap video memory b8 based on whether the next terminal is currently being displayed or not
    if (scheduled_term == visible_term){
        page_table_0[VIDEO_MEM_ADDR].addr_31_12 = VIDEO_MEM_ADDR;
        user_vidmap_table[VIDEO_MEM_ADDR].addr_31_12 = VIDEO_MEM_ADDR;
    }
    else{
        page_table_0[VIDEO_MEM_ADDR].addr_31_12 = VIDEO_MEM_ADDR + (scheduled_term+1);
        user_vidmap_table[VIDEO_MEM_ADDR].addr_31_12 = VIDEO_MEM_ADDR + (scheduled_term+1);
		pair _p = { .x = terms[scheduled_term].cursor.col, .y = terms[scheduled_term].cursor.row };
        _screen_seek(_p);
    }
    flush_tlb();

    //if terminal does not have a pid, means we need to launch shell! (bootup process)
    if(terms[scheduled_term].pid == -1){
        send_eoi(PIT_IRQ);
        sti();
        syscall_execute((uint8_t*)"shell");
    }
    //else, get pcb data, restore user page and vidmap
    pcb_t* next_pcb = (pcb_t*) (EIGHT_MB - (terms[scheduled_term].pid + 1)*EIGHT_KB);
    page_directory[USER_PAGE_INDEX].addr_31_12 = (EIGHT_MB + (next_pcb->process_id * FOUR_MB_PAGE)) >> ADDRESS_SHIFT_LEFT;
    flush_tlb();

    this_pcb = next_pcb;

    // prepare for contect switch (mat style) ------------------------------------------------------
    tss.esp0 = EIGHT_MB - ((next_pcb->process_id) * EIGHT_KB);
    tss.ss0 = KERNEL_DS;
    asm volatile ("\
        movl %0, %%ebp;  \
        movl %1, %%esp;" \
        :
        : "g"(next_pcb->scheduled_ebp), "g" (next_pcb->scheduled_esp)
    );

    send_eoi(PIT_IRQ);
    //page_table_0[VIDEO_MEM_ADDR].addr_31_12 = VIDEO_MEM_ADDR;
}
