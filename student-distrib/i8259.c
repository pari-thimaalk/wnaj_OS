/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 * info from: https://wiki.osdev.org/8259_PIC
 */

#include "i8259.h"
#include "lib.h"


/* from OSDev Wiki */
static inline void io_wait(void) {
    outb(0, 0x80);
}

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask = 0xFF; /* IRQs 0-7  */
uint8_t slave_mask = 0xFF;  /* IRQs 8-15 */

/* 
 * i8259_init
 *  DESCRIPTION: Mask all interrupts and initialize I8259
 *               Enable the interrupt of pin 2
 *  INPUTS: none
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: Enable the interrupt of pin 2
 */
void i8259_init(void) {

    // mask all the interrupts
    outb(master_mask, MASTER_8259_DATA); 
    outb(slave_mask, SLAVE_8259_DATA);   

    // begin initializing 8259
    outb(ICW1, MASTER_8259_PORT);
    outb(ICW1, SLAVE_8259_PORT);

    // map the registers locations
    outb(ICW2_MASTER, MASTER_8259_DATA);
    outb(ICW2_SLAVE, SLAVE_8259_DATA);

    // set up the slave PIC
    outb(ICW3_MASTER, MASTER_8259_DATA);
    outb(ICW3_SLAVE, SLAVE_8259_DATA);

    // select 8086 mode
    outb(ICW4, MASTER_8259_DATA);
    outb(ICW4, SLAVE_8259_DATA); 

    // restore masks
    outb(master_mask, MASTER_8259_DATA);
    outb(slave_mask, SLAVE_8259_DATA);
    
    enable_irq(SLAVE_IRQ_NUM); // enable the slave PIC
}

/* 
 * enable_irq
 *  DESCRIPTION: Enable (unmask) the specified IRQ
 *  INPUTS: irq_num: the irq to unmask
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: enable the respective irq
 */
void enable_irq(uint32_t irq_num) {
    uint16_t port;
    uint8_t value;
 
    if(irq_num < MAX_PORT) {
        port = MASTER_8259_DATA;  // set the port to the master if the irq is less than 8
    } else {
        port = SLAVE_8259_DATA;   // set the port to the slave if the irq is more than 8
        irq_num -= MAX_PORT;             // subtract by offset of 8 to get correct starting place for slave PIC
    }

    value = inb(port) & ~(1 << irq_num);   // set the corresponding bit to 0
    outb(value, port);
}

/* 
 * disable_irq
 *  DESCRIPTION: Disable (mask) the specified IRQ
 *  INPUTS: irq_num: the irq to mask
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: disable the respective irq
 */
void disable_irq(uint32_t irq_num) {
    uint16_t port;
 
	if (irq_num < 8) {
		port = PIC1_DATA;
	} else {
		port = PIC2_DATA;
		irq_num -= 8;
	}
	outb(inb(port) | (1 << irq_num), port); 
}

void disable_irq_all() {
	outb(0xff, PIC2_DATA);
	outb(0xff, PIC1_DATA);
}

/* 
 * send_eoi
 *  DESCRIPTION: Send end-of-interrupt signal for the specified IRQ
 *  INPUTS: irq_num: the irq for EOI signal
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: none
 */
void send_eoi(uint32_t irq_num) {
	/* if (irq_num >= 8) */
	/* 	outb(EOI, PIC2_COMMAND); */
	/* outb(EOI, PIC1_COMMAND); */

    if(irq_num >= MAX_PORT){
        outb((EOI | (irq_num-MAX_PORT)), SLAVE_8259_PORT);      //clear on secondary pic if greater then 7
        outb((EOI | 0x2), MASTER_8259_PORT);                    //clear on primary as well
    } else {
        outb((EOI | irq_num), MASTER_8259_PORT);                 //else, only clear primary
    }
}
