#include "types.h"
#include "console.h"
#include "gdt.h"
#include "memory.h"
#include "idt.h"
#include "timer.h"
#include "keyboard.h"
#include "process.h"
#include "thread.h"
#include "ide.h"
#include "fs.h"
#include "user.h"

void kernelMain(const void* multiboot_structure, uint32_t multiboot_magic)
{
    // 0. initialize console driver
    cslInit();
    printf("[ORCAS]: an evolving operating system.\n");
    printf("[ORCAS]: powered by arjenk.\n");

    // 1. initialize kernel memory
    kmemInit();
    // pageTest();
    // mallocTest();

    // 2. start segmentation and interrupts
    gdtInit();
    idtInit();

    // 3. initialize thread manager, for multitasking
    thrInit();
    // thrTest();
    // cvTest();
    // mtxTest();
    // semTest();

    // 4. initialize file system and shell
    fsInit();
    printf("arjenk@orcas: %s$ ", fileSys.cwd_name);

    // 5. initialize other device drivers, such as
    //    keyboard and PIT
    kbdDriverInit();
    timerDriverInit();

}