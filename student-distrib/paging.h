#ifndef PAGING
#define PAGING


#include "paging_ass.h"
#include "x86_desc.h"
#include "types.h"


#define NUM_PTEs 1024 //total 1024 entries in a page table
#define NUM_PDEs 1024 //1024 entries in a page table

#define PD_SIZE 0x1000 //4096 bytes
#define PT_SIZE 0x1000 //4096 bytes
#define PAGE_SIZE_4KB 0x1000 //4Kb
#define PAGE_SIZE_4MB 0x400000 //4Mb
#define alignment_4kb 4096
#define ZERO 0
#define ADDRESS_SHIFT_LEFT 12

#define PHYS_VIDMEM 0xB7000 //physical vidmem address. 0xb7000 will always point to physical b8000

#define USER_PAGE_INDEX 32
#define USER_PHYS_MEM 0x800000

#define USER_VIDMAP_PAGE_INDEX 33

#define KERNEL_PHYS_MEM 0x400000

#define VIDEO_MEM_ADDR 0xB8 // found from stack overflow: https://stackoverflow.com/questions/17367618/address-of-video-memory

#define PRESENT 1
#define NOT_PRESENT 0
#define USER 1
#define SUPERVISOR 0
#define READ_WRITE 1
#define READ_ONLY 0
#define FOUR_KB_PAGES 0
#define FOUR_MB_PAGES 1

/*struct for page directory entry. bit fields derived from OSDev paging link*/
typedef struct __attribute__ ((packed)) page_dir_entry_desc_t {
    uint32_t present: 1;
    uint32_t read_write: 1;
    uint32_t user_supervisor: 1;
    uint32_t write_through: 1;
    uint32_t cache_disabled: 1;
    uint32_t accessed: 1;
    uint32_t dirty: 1;
    uint32_t page_size: 1;
    uint32_t global: 1;
    uint32_t avail: 3;
    uint32_t addr_31_12: 20;
} page_dir_entry_desc_t;

/*struct for page table entry. bit fields derived from OSDev paging*/
typedef struct __attribute__ ((packed)) page_table_entry_desc_t {
    uint32_t present: 1;
    uint32_t read_write: 1;
    uint32_t user_supervisor: 1;
    uint32_t write_through: 1;
    uint32_t cache_disabled: 1;
    uint32_t accessed: 1;
    uint32_t dirty: 1;
    uint32_t page_attr_table: 1;
    uint32_t global: 1;
    uint32_t avail: 3;
    uint32_t addr_31_12: 20;
} page_table_entry_desc_t;

/*array for page directory*/
page_dir_entry_desc_t page_directory[NUM_PDEs] __attribute__((aligned (alignment_4kb)));
/*array for zeroeth page table*/
page_table_entry_desc_t page_table_0[NUM_PTEs] __attribute__((aligned (alignment_4kb)));
/*array for vidmap page table*/
page_table_entry_desc_t user_vidmap_table[NUM_PTEs] __attribute__((aligned (alignment_4kb)));

extern void init_paging();
extern void set_pde (int present, int read_write, int user_super, int page_size, int index, uint32_t start_phys_address);
extern void set_pte (int present, int read_write, int user_super, int index, page_table_entry_desc_t* table);

#endif
