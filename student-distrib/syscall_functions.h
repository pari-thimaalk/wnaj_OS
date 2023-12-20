#include "types.h"
#include "lib.h"
#include "x86_desc.h"
#include "filesystem.h"
#include "paging.h"



//relevant data structures have been moved to filesystem.h

//function prototypes for syscalls matches ece391syscall signature
int32_t syscall_halt (uint8_t status);
int32_t syscall_execute (const uint8_t* command);
int32_t syscall_read (int32_t fd, void* buf, int32_t nbytes);
int32_t syscall_write (int32_t fd, const void* buf, int32_t nbytes);
int32_t syscall_open (const uint8_t* filename);
int32_t syscall_close (int32_t fd);
int32_t syscall_getargs(uint8_t* buf, int32_t nbytes);
int32_t syscall_vidmap(uint8_t** screen_start);
int32_t syscall_sethandler(int32_t signum, void* handler_address);
int32_t syscall_sigreturn(void);

int32_t dummy_read(int32_t fd, void* buf, int32_t nbytes);
int32_t dummy_write(int32_t fd, const void* buf, int32_t nbytes);

