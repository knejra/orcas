#include "gdt.h"
.extern handleInterrupt
.globl interrupt
interrupt:
  pushl %ds
  pushl %es
  pushl %fs
  pushl %gs
  pushal
  
  # load kernel data segment
  movw $16, %ax
  movw %ax, %ds
  movw %ax, %es

  pushl %esp # parameter in %esp
  call handleInterrupt
  addl $4, %esp 

.globl trapret
trapret:
  popal
  popl %gs
  popl %fs
  popl %es
  popl %ds
  addl $0x8, %esp
  sti
  iret
