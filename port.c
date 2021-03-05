#include "port.h"

uint8_t inb(uint16_t port)
{
    uint8_t data;
    asm volatile("in %1, %0" : "=a"(data) : "d"(port));
    return data;
}

void outb(uint16_t port, uint8_t data)
{
    asm volatile("out %0, %1" : : "a"(data), "d"(port));
}

void insl(uint16_t port, void *addr, int cnt)
{
    asm volatile("cld; rep insl" :
                 "=D" (addr), "=c" (cnt) :
                 "d" (port), "0" (addr), "1" (cnt) :
                 "memory", "cc");
}

void outsl(uint16_t port, void *addr, int cnt)
{
    asm volatile("cld; rep outsl" :
                 "=S"(addr), "=c" (cnt) :
                 "d" (port), "0"(addr), "1" (cnt) :
                 "cc");
}