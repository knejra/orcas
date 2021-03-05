#include "timer.h"
#include "port.h"
#include "console.h"

uint32_t ticks = 0;

void timerDriverInit()
{
    uint32_t s = FREQUENCY / SET_FREQ;

    // set command byte
    outb(PIT_CMD_PORT, 0x36);
    
    uint8_t lowbit = (uint8_t) (s & 0xff);
    uint8_t highbit = (uint8_t) ((s >> 8) & 0xff);

    outb(CHAN0_DATA_PORT, lowbit);
    outb(CHAN0_DATA_PORT, highbit);
}

void timerInterruptHandler()
{
    ticks++;
}