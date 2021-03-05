#ifndef _CONSOLE_H
#define _CONSOLE_H

#include "concurrency.h"
#include "types.h"

#define CSL_WIDTH  80
#define CSL_HEIGHT 25

#define CSL_PORT_CMD  0x3d4
#define CSL_PORT_DATA 0x3d5

int row, col;
uint16_t *videoMemory;
spinlock_t cslLock;

// clear: clear console
// parameters: void 
// outputs   : void
void clear();

// getCursor: get current cursor
// parameters: x-x axis position
//             y-y axits position
// outputs   : void
void getCursor(int *x, int *y);

// setCursor: set current cursor
// parameters: x-x axis position to set
//             y-y axis postion to set
// outputs   : void
void setCursor(int x, int y);

// getPos: get current position
// parameters: x-out param x
//             y-out param y
// outputs   : void
void getPos(int *x, int *y);

// setPos: put c at pos(x, y)
// parameters: x-x axis position
//             y-y axis position
//             c-character to set
// outputs   : void 
void setPos(int x, int y, char c);

// scroll: scroll up the screen
// parameters: void
// outputs   : void
void scroll();

// putc: print character
// parameters: c-character
// outputs   : void
void putc(char c);

// print: print string
// parameters: str-string
// outputs   : void
void puts(char *str);

// printint: print integer
// parameters: num-integer
// outputs   : void
void printi(int num, int base);

// printf: print as format
// parameters: format-format to print
//             args-arguments
// outputs   : void
void printf(const char *format, ...);

// cslInit: initialize console driver
// parameters: void
// outputs   : void
void cslInit();

// Test: console test
void cslTest();


#endif // _CONSOLE_H