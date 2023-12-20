#define ASM     1

#our wrapper functions
.global rtc_linkage, keyboard_linkage, mouse_linkage, syscall_linkage, pit_linkage

#global variable

syscall_jumptable:
    .long 0x0, syscall_halt, syscall_execute, syscall_read, syscall_write, syscall_open, syscall_close, syscall_getargs, syscall_vidmap, syscall_sethandler, syscall_sigreturn

store_return:   .long 0x0

rtc_linkage:
    pushal           #our big 7 registers get saved
    pushfl           #our flags get saved
    call rtc_handler
    popfl
    popal
    iret

pit_linkage:
    pushal           #our big 7 registers get saved
    pushfl           #our flags get saved
    call pit_handler
    popfl
    popal
    iret

keyboard_linkage:
    pushal           #our big 7 registers get saved
    pushfl           #our flags get saved
    call keyboard_handler
    popfl
    popal
    iret

mouse_linkage:
    pushal           #our big 7 registers get saved
    pushfl           #our flags get saved
    call mouse_handler
    popfl
    popal
    iret

syscall_linkage:
    # we first check for argument number, make sure it is within our jumptable
    cmpl $10,%eax #we do 10 because we have 10 system calls
    ja   invalid_cmd
    cmpl $0,%eax
    jbe  invalid_cmd

    # push registers and flags to the stack
    pushal
    #pushfl

    # push the function arguments onto stack, in reverse order
    pushl %edx
    pushl %ecx
    pushl %ebx

    call *syscall_jumptable(,%eax,4)    # we jump to function in the jump table
    addl $12, %esp
    movl %eax,store_return  #prevent from popal clobbering eax value

    #popfl
    popal
    movl store_return,%eax
    jmp done

invalid_cmd:
    movl $-1,%eax

done:
    iret



