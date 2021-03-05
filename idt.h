#ifndef _IDT_H
#define _IDT_H

#include "types.h"

#define IDTSIZE                     256

#define IDX_SYSCALL                 128

#define IT_INTERRUPT                0x8e
#define IT_TRAP                     0x8f
#define IT_SYSCALL                  0xef

#define IV_DEVIDE_ERROR                0
#define IV_DEBUG                       1
#define IV_NMI                         2
#define IV_BREAKPOINT                  3
#define IV_OVERFLOW                    4
#define IV_BOUND_CHECK                 5
#define IV_INVALID_OPCODE              6
#define IV_DEVICE_NOT_AVAILABLE        7
#define IV_DOUBLE_FAULT                8
#define IV_COPROCESSOR_SEG_OVERRUN     9
#define IV_INVALID_TSS                10
#define IV_SEGMENT_NOT_PRESENT        11
#define IV_STACK_SEGMENT              12
#define IV_GENERAL_PROTECTION         13
#define IV_PAGE_FAULT                 14
#define IV_INTEL_RESERVED             15
#define IV_FLOATING_POINT_ERROR       16
#define IV_ALIGNMENT_CHECK            17

// slave pic handled interrupt: 0x28 (40)
#define MASTER_BOUND                  0x20
#define SLAVE_BOUND                   0x28 

#define EOI                           0x20

#define IV_TIMER                      32
#define IV_KEYBOARD                   33
#define IV_PIC_CASCADE                34
#define IV_NETWORK                    35
#define IV_MOUSE                      36
#define IV_IDE                        46

#define IV_SYSCALL                    0x80

#define IV_TEST_CODE                   3

struct ide_t
{
    uint16_t offsetlow16; 
    uint16_t selector; 
    uint8_t reserved;
    uint8_t privilege;
    uint16_t offsethigh16;
}__attribute__((packed));

// learn from xv6
struct trapframe 
{
  // registers as pushed by pusha
  uint32_t edi;
  uint32_t esi;
  uint32_t ebp;
  uint32_t oesp;      // useless & ignored
  uint32_t ebx;
  uint32_t edx;
  uint32_t ecx;
  uint32_t eax;

  // rest of trap frame
  uint16_t gs;
  uint16_t padding1;
  uint16_t fs;
  uint16_t padding2;
  uint16_t es;
  uint16_t padding3;
  uint16_t ds;
  uint16_t padding4;
  uint32_t trapno;

  // below here defined by x86 hardware
  uint32_t err;
  uint32_t eip;
  uint16_t cs;
  uint16_t padding5;
  uint32_t eflags;

  // below here only when crossing rings, such as from user to kernel
  uint32_t esp;
  uint16_t ss;
  uint16_t padding6;
};


typedef struct ide_t ide_t;
ide_t idt[IDTSIZE];

// sti: open interrupt
// parameters: void
// outputs   : void
void sti();

// cli: close interrupt
// parameters: void
// outputs   : void
void cli();

// idinit: init an interrupt descriptor
// parameters: id-interrupt descriptor
//             offset-interrupt vector offset
//             selector-segment selector
//             privilege-interrupt privilege
// outpus    : void
void idInit(ide_t *id, uint32_t offset, uint16_t selector, uint8_t privilege);

// idtinit: init interrupt descriptor table
// parameters: void
// outputs   : void
void idtInit();

// interrupt: do interrupt
// parameters: interruptNum-interrupt number
// outputs   : void
void handleInterrupt(struct trapframe *tf);

#endif // _IDT_H