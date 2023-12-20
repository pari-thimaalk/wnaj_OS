#include "idt.h"
#include "ass_link_handler.h"

/* 
 * init_idt
 *  DESCRIPTION: Initializes the idt
 *  INPUTS: None
 *  OUTPUTS: None
 *  RETURN VALUE: None
 *  SIDE EFFECTS: will set up idt table, allowing us to handle exceptions and interrupts
 */
void init_idt(){
    int i;
    //we only set exceptions until 0x13, leave 0x14 to x1F alone
    for(i = 0; i < 20; i++){
        add_exception(i);
    }
    for(i = 32; i < NUM_VEC; i++){
        add_interrupt(i);
    }
    //set system call entry into idt, change dpl level to 3 as it is user level
    idt[0x80].dpl = 0x3;
    //set idt entries 0x00-0x13
    SET_IDT_ENTRY(idt[0], divide_error);
    SET_IDT_ENTRY(idt[1], debug_exception);
    SET_IDT_ENTRY(idt[2], nmi_exception);
    SET_IDT_ENTRY(idt[3], breakpoint_exception);
    SET_IDT_ENTRY(idt[4], overflow_exception);
    SET_IDT_ENTRY(idt[5], oob_exceeeded);
    SET_IDT_ENTRY(idt[6], invalid_opcode);
    SET_IDT_ENTRY(idt[7], device_unavailable);
    SET_IDT_ENTRY(idt[8], double_fault);
    SET_IDT_ENTRY(idt[9], segment_overrun);
    SET_IDT_ENTRY(idt[10], invalid_tss);
    SET_IDT_ENTRY(idt[11], segment_not_present);
    SET_IDT_ENTRY(idt[12], stack_segment_fault);
    SET_IDT_ENTRY(idt[13], general_protection_fault);
    SET_IDT_ENTRY(idt[14], page_fault);
    SET_IDT_ENTRY(idt[16], floating_pt_exception);
    SET_IDT_ENTRY(idt[17], alignment_check);
    SET_IDT_ENTRY(idt[18], machine_check);
    SET_IDT_ENTRY(idt[19], simd_floating_pt_exception);
    //adding rtc exception, irq 8 + 32 = 40
    //set up interrupts for keybaord and rtc
    SET_IDT_ENTRY(idt[32], pit_linkage);
    SET_IDT_ENTRY(idt[33], keyboard_linkage);
    SET_IDT_ENTRY(idt[40], rtc_linkage);
    SET_IDT_ENTRY(idt[0x2C], mouse_linkage);//0x2c = 44 = 32 + 12 = IRQ 12
    SET_IDT_ENTRY(idt[0x80],syscall_linkage);
    lidt(idt_desc_ptr);
}

//helper function to add an exception
void add_exception(int exception_number){
    //enable 32-bit trap gate, osdev
    idt[exception_number].present = 1;
    idt[exception_number].reserved4 = 0;
    idt[exception_number].reserved3 = 1;
    idt[exception_number].reserved2 = 1;
    idt[exception_number].reserved1 = 1;
    idt[exception_number].reserved0 = 0;
    idt[exception_number].size = 1;
    idt[exception_number].dpl = 0;           
    idt[exception_number].seg_selector = KERNEL_CS;
}

//helper function to add an interrupt
void add_interrupt(int interrupt_number){
    //enable 32-bit interrupt gate, osdev
    idt[interrupt_number].present = 1;
    idt[interrupt_number].reserved4 = 0;
    idt[interrupt_number].reserved3 = 0;
    idt[interrupt_number].reserved2 = 1;
    idt[interrupt_number].reserved1 = 1;
    idt[interrupt_number].reserved0 = 0;
    idt[interrupt_number].size = 1;
    idt[interrupt_number].seg_selector = KERNEL_CS;
    idt[interrupt_number].dpl = 0;
}
