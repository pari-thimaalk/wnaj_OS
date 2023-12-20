#include "mouse.h"
#include "term.h"
#include "rtc.h"

volatile uint8_t mouse_packets[3];
volatile int left_btn = 0;
volatile int right_btn = 0;
int i = -1;

// int8_t* kalbaking_1 = "Can you think about the answer or can you not";
// int8_t* kalbaking_2 = "You guys you complained a lot";
// int8_t* kalbaking_3 = "I give this question, and they still mess it up";
// int8_t* kalbaking_4 = "Everybody is nice in principle";
// int8_t* kalbaking_5 = "That was joke everyone";
// int8_t* kalbaking_6 = "All that time spent complaining could be coding";
// int8_t* kalbaking_7 = "Dont memorise this; You will suffer and learn";
// int8_t* kalbaking_8 = "You are making fun of me";
// int8_t* kalbaking_9 = "Elementary schoolers can do this";
// int8_t* kalbaking_10 = "If I give you CD, you cant even read CD anymore";

// char* arr[] = {"foo", "dei"};

char* kalbaking[] = {"Can you think about the answer or can you not", 
                     "You guys you complained a lot",
                     "I give this question, and they still mess it up",
                     "Everybody is nice in principle",
                     "That was joke everyone",
                     "All that time spent complaining could be coding",
                     "Dont memorise this; You will suffer and learn",
                     "You are making fun of me",
                     "Elementary schoolers can do this",
                     "If I give you CD, you cant even read CD anymore"};



void mouse_init(){
    //enabling irq12 is a lengthy process for some reason
    //On some systems, the PS2 aux port is disabled at boot.
    outb(ENABLE_AUX_DEV, MOUSE_IO_64);
    outb(GET_CQ_STATUS,MOUSE_IO_64);

    //After you get the Status byte, you need to set bit number 1 (value=2, Enable IRQ12), and clear bit number 5 (value=0x20, Disable Mouse Clock)
    //Then send command byte 0x60 ("Set Compaq Status") to port 0x64, followed by the modified Status byte to port 0x60.
    uint8_t stat = (inb(SET_CQ_STATUS) | 0x02) & ~(0x20);

    //osdev wants to disable mouse clock but i only get interrupts if i leave it enabled. WHY????
    // uint8_t stat = (inb(SET_CQ_STATUS) | 0x02);
    outb(SET_CQ_STATUS,MOUSE_IO_64);
    outb(stat,MOUSE_IO_60);

    //disable streaming and set packet rate to 100/s, starts sending packets when mouse is moved or clicked
    mouse_write(0xF6);
    mouse_write(0xF4);

    enable_irq(MOUSE_IRQ);
}

void mouse_handler(){
    cli();
    uint8_t packet = inb(0x60);

    
    if(i >= 0){
        switch (i)
        {
        case 0:
            mouse_packets[0] = packet;
            if(!left_btn && (mouse_packets[0] & 0x01)){left_btn = 1; status_puts(kalbaking[get_randomizer()], 1); delay = 3*512;}
            if(left_btn && !(mouse_packets[0] & 0x01)){left_btn = 0;}
            if(!right_btn && (mouse_packets[0] & 0x02)){right_btn = 1; status_puts("Imagine not taking 391 as a Sophomore", 1); delay = 3*512;}
            if(right_btn && !(mouse_packets[0] & 0x02)){right_btn = 0;}
            break;
        case 1:

            break;
        case 2:

            break;
        default:
            PRINTLN("Something weird in mouse");
            break;
        }
        
    }
    
    //mouse packets are sent in 3 bytes, reset after each group
    i++;
    i %= 3;
    send_eoi(MOUSE_IRQ);
    sti();
}

//All output to port 0x60 or 0x64 must be preceded by waiting for bit 1 (value=2) of port 0x64 to become clear.
void mouse_wait_write(){
    int time_down = 0;
    while(time_down < 100000){
        time_down++;
        if(!(inb(MOUSE_IO_64) | 0x02)){break;}
    }
    return;
}
//Similarly, bytes cannot be read from port 0x60 until bit 0 (value=1) of port 0x64 is set.
void mouse_wait_read(){
    int time_down = 0;
    while(time_down < 100000){
        time_down++;
        if((inb(MOUSE_IO_64) | 0x01)){break;}
    }
    return;
}
/*
    to write bytes to mouse port:
    wait and send 0xd4
    wait and send byte
    wait and receive ack before moving on
*/


void mouse_write(uint8_t byte){
  mouse_wait_write();
  outb(0xD4, 0x64);
  mouse_wait_write();
  outb(byte, 0x60);
  mouse_wait_read();
  inb(MOUSE_IO_60);
}

