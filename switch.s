.globl ctxSwitch
.func ctxSwitch
ctxSwitch:
  movl 4(%esp), %eax
  movl 8(%esp), %edx
  
  pushl %ebp
  pushl %ebx
  pushl %esi
  pushl %edi

  movl %esp, (%eax)
  movl %edx, %esp

  popl %edi
  popl %esi
  popl %ebx
  popl %ebp

  ret
.endfunc

.globl thread_entry
.func thread_entry
thread_entry:
  ret
.endfunc

