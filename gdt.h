#ifndef _GDT_H
#define _GDT_H

#include "types.h"

enum SEG
{
    SEG_RESERVED,
    SEG_KERNEL_CODE,
    SEG_KERNEL_DATA,
    SEG_USER_CODE,
    SEG_USER_DATA,
    SEG_GLOBL_TSS
};

#define GDTSIZE 6

#define ST_KERNEL_CODE 0X9a
#define ST_KERNEL_DATA 0x92
#define ST_USER_CODE   0Xfa  
#define ST_USER_DATA   0xf2

struct gde_t
{
    uint16_t limitlow16;
    uint16_t baselow16;
    uint8_t basemid8;
    uint8_t privilege;
    uint8_t flagsAndLimitHigh4;
    uint8_t basehigh8;
}__attribute__((packed));

typedef struct gde_t gde_t;
gde_t gdt[GDTSIZE];

// gdeInit: init a global descriptor
// parameters: gd-global descriptor
//             limit-segment limit
//             base-segment base
//             privilege-privilege
// outputs  :  void
void gdeInit(gde_t *gd, uint32_t limit, uint32_t base, uint8_t privilege);

// gdtInit: init global descriptor table
// parameters: void
// outputs   : void
void gdtInit();

// selector: transfer global descriptor table index into segment selector
// parameters: index-global descriptor table index
uint16_t selector(uint16_t index);

#endif /* _GDT_H */