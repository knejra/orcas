#ifndef _TIMER_H
#define _TIMER_H

#define CHAN0_DATA_PORT 0x40
#define CHAN1_DATA_PORT 0x41
#define CHAN2_DATA_PORT 0x42
#define PIT_CMD_PORT    0x43

#define FREQUENCY 1193180
#define SET_FREQ  100

// timerDriverInit: initialize timer
// parameters: void
// outputs   : void
void timerDriverInit();

// timerInterruptHandler: handle timer interrupt
// parameters: void
// outpus    : void
void timerInterruptHandler();

#endif // _TIMER_H