#include "types.h"

/*
* enable_paging
* description: enables paging by setting cr3, cr4, cr0
* arguments: pointer to page table
*/
extern void enable_paging(uint32_t address);
/*
* flush_tlb
* description: flushes the tlbs
*/
extern void flush_tlb();
