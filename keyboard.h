#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "types.h"

#define KBD_DATA_PORT   0x60
#define KBD_CMD_PORT    0x64
// data in buffer
#define KBD_DATA_INBUF  0x01

#define HOME        0xE0
#define END         0xE1
#define UP          0xE2
#define DOWN        0xE3
#define LEFT        0xE4
#define RIGHT       0xE5
#define PGUP        0xE6
#define PGDN        0xE7
#define INS         0xE8
#define DEL         0xE9

static uint8_t keymap[256] =
{
    0,   0x1B, '1',  '2',  '3',  '4',  '5',  '6', 
  '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
  'q',  'w',  'e',  'r',  't',  'y',   'u',  'i', 
  'o',  'p',  '[',  ']',  '\n',   0,   'a',  's',
  'd',  'f',  'g',  'h',  'j',  'k',   'l',  ';',
  '\'', '`',    0,   '\\', 'z',  'x',  'c',  'v',
  'b',  'n',  'm',  ',',  '.',  '/',    0,   '*',
    0,  ' ',    0,    0,    0,    0,    0,     0,
    0,    0,    0,    0,    0,    0,    0,   '7', 
  '8',  '9',  '-',  '4',  '5',  '6',  '+',   '1',
  '2',  '3',  '0',  '.',    0,    0,    0,     0,   
  [0x9C] '\n', [0xB5] '/', [0xC8] UP, [0xD0] DOWN, 
  [0xC9] PGUP,  [0xD1] PGDN, [0xCB] LEFT, [0xCD] RIGHT,
  [0x97] HOME,  [0xCF] END, [0xD2] INS, [0xD3] DEL
};

// kbdDriverInit: initialize keyboard interrupt
// parameters: void
// outputs   : void
void kbdDriverInit();

// getc: get a character from console
// parameters: void
// outputs   : return char
int getc();

// gets: get a string from console
// parameters: str-out param string
// outputs   : length of the string
int gets(char *str);

char line[50], cmd[50], arg[50];
int pos;
// parseCmd: parse command line
// parameters: line-line
//             cmd-command
//             args-params
// outputs   : void
void parseCmd(char *line, char *cmd, char *args);

// kbdInterruptHandler: keyboard interrupt handler
// parameters: void
// outputs   : void
void kbdInterruptHandler();

#endif // _KEYBOARD_H