#include "paging.h"
#include "types.h"

/*
* init_paging
* description: initialising paging and setting params in control regs using asm linkage
*/
void init_paging() {
    int i;
    /*set all entries in page table 0 and user vidmap page to not presnt*/
    for (i = 0; i < NUM_PTEs; i++) {
        set_pte(NOT_PRESENT, READ_WRITE, SUPERVISOR, i, page_table_0);
        set_pte(NOT_PRESENT, READ_WRITE, USER, i, user_vidmap_table);
    }
    /*set entry for video mem addr in page table 0*/
    set_pte(PRESENT, READ_WRITE, SUPERVISOR, VIDEO_MEM_ADDR, page_table_0);
    //background vidpages for each terminal - b9 for terminal 1, ba for terminal 2, bb for terminal 3
    set_pte(PRESENT, READ_WRITE, SUPERVISOR, 0xB9, page_table_0);
    set_pte(PRESENT, READ_WRITE, SUPERVISOR, 0xBA, page_table_0);
    set_pte(PRESENT, READ_WRITE, SUPERVISOR, 0xBB, page_table_0);
    set_pte(PRESENT, READ_WRITE, SUPERVISOR, PHYS_VIDMEM >> 12, page_table_0);
    page_table_0[PHYS_VIDMEM >> 12].addr_31_12 = VIDEO_MEM_ADDR;

    //set entry for video mem addr in user space
    set_pte(PRESENT, READ_WRITE, USER, VIDEO_MEM_ADDR, user_vidmap_table);
    
    /*set entry for first 0-4mb in page directory*/
    set_pde(PRESENT, READ_WRITE, SUPERVISOR, FOUR_KB_PAGES, 0, ZERO);
    /*set entry for first 4-8mb in page directory for kernel stuff*/
    set_pde(PRESENT, READ_WRITE, SUPERVISOR, FOUR_MB_PAGES, 1, KERNEL_PHYS_MEM >> ADDRESS_SHIFT_LEFT);

    /*set address for 0th entry in PD to point to PT0*/
    page_directory[0].addr_31_12 |= ((uint32_t) page_table_0 >> ADDRESS_SHIFT_LEFT);

    /*set all other entries in PD to not present*/
    for (i = 2; i < NUM_PDEs; i++){
        set_pde(NOT_PRESENT, READ_WRITE, SUPERVISOR, FOUR_MB_PAGES, i, ZERO);
    }

    //setting entry for user program page
    set_pde(PRESENT, READ_WRITE, USER, FOUR_MB_PAGES, USER_PAGE_INDEX, USER_PHYS_MEM >> ADDRESS_SHIFT_LEFT);
    //setting entry for vidmap table
    set_pde(PRESENT, READ_WRITE, USER, FOUR_KB_PAGES, USER_VIDMAP_PAGE_INDEX, (uint32_t) (user_vidmap_table) >> ADDRESS_SHIFT_LEFT);

    /*enable paging using assembly function*/
    enable_paging((uint32_t) page_directory);
        
}   

/*
* set_pde
* sets entry of pde
* args: present, read_write, user_super, page_size, index, base phys address
*/
void set_pde (int present, int read_write, int user_super, int page_size, int index, uint32_t start_phys_address) {
    page_directory[index].present = present;
    page_directory[index].read_write = read_write;
    page_directory[index].user_supervisor = user_super;
    page_directory[index].page_size = page_size;
    page_directory[index].write_through = 0;
    page_directory[index].cache_disabled = 0;
    page_directory[index].accessed = 0;
    page_directory[index].dirty = 0;
    page_directory[index].global = 0;
    page_directory[index].avail = 0;
    page_directory[index].addr_31_12 = start_phys_address;
}

/*
* set_pte
* sets entry of pte
* args: present, read_write, user_super, index, page table
*/
void set_pte (int present, int read_write, int user_super, int index, page_table_entry_desc_t* table) {
    table[index].present = present;
    table[index].read_write = read_write;
    table[index].user_supervisor = user_super;
    table[index].write_through = 0;
    table[index].cache_disabled = 0;
    table[index].accessed = 0;
    table[index].dirty = 0;
    table[index].page_attr_table = 0;
    table[index].global = 0;
    table[index].avail = 0;
    table[index].addr_31_12 = index;
}
