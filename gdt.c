#include "gdt.h"

void gdeInit(gde_t *gd, uint32_t limit, uint32_t base, uint8_t privilege)
{
    uint8_t *target = (uint8_t *)gd;
 
    if(limit > 0xffff)
    {
        // for paging, a page size is 4K = 0xfff
        limit = limit >> 12; 
        // set G = 1, X = 1, 32bits paging segment
        target[6] = 0xc0;
    }
    else
    {
        // smaller than a page, G = 0, X = 1, 32bits un-page segment
        target[6] = 0x40;
    }
    
    target[0] = limit & 0xff;
    target[1] = (limit >> 8) & 0xff;
    target[2] = base & 0xff;
    target[3] = (base >> 8) & 0xff;
    target[4] = (base >> 16) & 0xff;
    target[5] = privilege;
    target[6] |= (limit >> 16) & 0xf;
    target[7] = (base >> 24) & 0xff;

}

void gdtInit()
{
    gdeInit(&(gdt[SEG_KERNEL_CODE]), 0xffffffff, 0, ST_KERNEL_CODE);
    gdeInit(&(gdt[SEG_KERNEL_DATA]), 0xffffffff, 0, ST_KERNEL_DATA);
    gdeInit(&(gdt[SEG_USER_CODE]), 0xffffffff, 0, ST_USER_CODE);
    gdeInit(&(gdt[SEG_USER_DATA]), 0xffffffff, 0, ST_USER_DATA);

    uint16_t gdtr[3];
    gdtr[0] = sizeof(gdt) - 1;
    gdtr[1] = (uint32_t)gdt;
    gdtr[2] = ((uint32_t)gdt) >> 16;

    asm volatile("lgdt (%0)" : : "r"(gdtr));
}

uint16_t selector(uint16_t index)
{
    return (index << 3);
}