#define ASM 1

.globl enable_paging
.align	4;

enable_paging:
    pushl	%ebp
    movl	%esp, %ebp
    pushl	%ebx
    pushl	%esi
    pushl	%edi

    movl	8(%ebp), %eax
    movl	%eax, %cr3

    movl    %cr4, %eax
    orl     $0x00000010, %eax #enable mixed page sizes (4mb and 4kb)
    movl	%eax, %cr4

    movl	%cr0, %eax
    orl		$0x80000001, %eax #enable paging. Numbers derived from osdev
    movl	%eax, %cr0

    call flush_tlb

    popl	%edi
    popl	%esi
    popl	%ebx
    leave
    ret

.globl	flush_tlb
.align	4

flush_tlb:
    movl	%cr3, %eax
    movl    %eax, %cr3
    ret

