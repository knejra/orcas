.set magic, 0x1badb002
.set flags, 0
.set checksum, -(magic + flags)

.section .multiboot
    .long magic
    .long flags
    .long checksum

.section .text
.extern kernelMain
.global loader

loader:
    mov $kernel_stack, %esp
    push %eax
    push %ebx
    call kernelMain

_stop:
    cli
    hlt
    jmp _stop


.section .bss
.space 2*1024*1024; #2 MB
kernel_stack: