#include "idt.h"
#include "gdt.h"
#include "console.h"
#include "keyboard.h"
#include "timer.h"
#include "port.h"
#include "process.h"
#include "thread.h"
//#include "fs.h"
#include "concurrency.h"
#include "syscall.h"

extern uint32_t interruptVectors[IDTSIZE];

void sti()
{
    asm volatile("sti");
}

void cli()
{
    asm volatile("cli");
}

void idInit(ide_t *id, uint32_t offset, uint16_t selector, uint8_t privilege)
{
    id->offsetlow16 = offset & 0xffff;
    id->selector = selector;
    id->reserved = 0;
    id->privilege = privilege;
    id->offsethigh16 = (offset >> 16) & 0xffff;
}

void idtInit()
{
    for(int i = 0; i < IDTSIZE; i++)
    {
        if(i == IDX_SYSCALL)
        {
            idInit(&(idt[i]), interruptVectors[i], selector(SEG_KERNEL_CODE), IT_INTERRUPT);
        }
        else
        {
            idInit(&(idt[i]), interruptVectors[i], selector(SEG_KERNEL_CODE), IT_INTERRUPT);
        }
    }

    uint16_t idtr[3];
    idtr[0] = sizeof(idt) - 1;
    idtr[1] = (uint32_t)idt;
    idtr[2] = ((uint32_t)idt) >> 16;

    // master PIC edge trigger mode
    outb(PIC_MASTER_CMD, 0x11);
    // map IRQ 0-7 to int 0x20-0x27
    outb(PIC_MASTER_DATA, 0x20);
    // link by IRQ 2
    outb(PIC_MASTER_DATA, 0x04);
    // no buffer
    outb(PIC_MASTER_DATA, 0x01);

    // slave PIC edge trigger mode
    outb(PIC_SLAVE_CMD, 0x11);
    // map IRQ 8-15 to int 0x28-0x2f
    outb(PIC_SLAVE_DATA, 0x28);
    // link master by IRQ 2
    outb(PIC_SLAVE_DATA, 0x02);
    // no buffer
    outb(PIC_SLAVE_DATA, 0x01);

    // allow receiving interrupt
    outb(PIC_MASTER_DATA, 0x00);
    outb(PIC_SLAVE_DATA, 0x00);

    asm volatile("lidt (%0)" : : "r"(idtr));

}

void handleInterrupt(struct trapframe *tf)
{
    //send EOI to pic
    if(tf->trapno >= MASTER_BOUND)
    {
        outb(PIC_MASTER_CMD, EOI);
        if(tf->trapno >= SLAVE_BOUND)
        {
            outb(PIC_SLAVE_CMD, EOI);
        }
    }

    switch (tf->trapno)
    {
    case IV_KEYBOARD:
    {
        kbdInterruptHandler();
        break;
    }
    case IV_TIMER:
    {
        //yield();
        thrYeild();
        break;
    }
    case IV_IDE:
    {
        //hardInterruptHandler();
        break;
    }
    case IV_SYSCALL:
    {
        //syscall();
        break;
    }
    default:
        break;
    }

}