#include "x86_desc.h"
#include "e_handler.h"

void init_idt();
void add_exception(int exception_number);
void add_interrupt(int interrupt_number);
