#include "console.h"
#include "port.h"
#include "util.h"

extern int row = 0;
extern int col = 0;
extern uint16_t *videoMemory= (uint16_t *)0xb8000;

void clear()
{
    for(int c = 0; c < CSL_WIDTH; c++)
    {
        for(int r = 0; r < CSL_HEIGHT; r++)
        {
            videoMemory[CSL_WIDTH * r + c] = (videoMemory[CSL_WIDTH * r + c] & 0xff00) | ' ';
        }
    }

    row = 0;
    col = 0;

    // spinlockLock(&cslLock);
    // row = 0;
    // col = 0;
    // spinlockUnlock(&cslLock);
}

void getCursor(int *x, int *y)
{
    outb(CSL_PORT_CMD, 14);
    int offset = inb(CSL_PORT_DATA) << 8;
    outb(CSL_PORT_CMD, 15);
    offset += inb(CSL_PORT_DATA);
    *x = offset / 80;
    *y = offset % 80;
}

void setCursor(int x, int y)
{
    int offset = x * 80 + y;
    outb(CSL_PORT_CMD, 14);
    outb(CSL_PORT_DATA, (offset >> 8) & 0xff);
    outb(CSL_PORT_CMD, 15);
    outb(CSL_PORT_DATA, offset & 0xff);
}

void getPos(int *x, int *y)
{
    spinlockLock(&cslLock);
    *x = row;
    *y = col;
    spinlockUnlock(&cslLock);
}

void setPos(int x, int y, char c)
{
    spinlockLock(&cslLock);
    row = x;
    col = y;
    videoMemory[CSL_WIDTH * row + col] = (videoMemory[CSL_WIDTH * row + col] & 0xff00) | c;
    spinlockUnlock(&cslLock);
}

void scroll()
{
    if(row >= CSL_HEIGHT)
    {
        for(int i = 0; i < CSL_HEIGHT * CSL_WIDTH; i++)
        {
            videoMemory[i] = videoMemory[i + CSL_WIDTH];
        }

        row = CSL_HEIGHT - 1;
    }
}

void putc(char c)
{
    videoMemory[CSL_WIDTH * row + col] = (videoMemory[CSL_WIDTH * row + col] & 0xff00) | (c == '\n' ? ' ' : c);
    col++;
    if(col > CSL_WIDTH || c == '\n')
    {
        row++;
        col = 0;
    }
    if(row > CSL_HEIGHT)
    {
        //scroll();
        clear();
    }
}

void puts(char *str)
{
    for(int i = 0; str[i] != '\0'; i++)
    {
        putc(str[i]);
    }
}

void printi(int num, int base)
{
    static char digits[] = "0123456789abcdef";
    char buf[32];

    if(num == 0)
    {
        putc('0');
        return ;
    }

    int i = 0, neg = 0;
    if(num < 0)
    {
        num = -num;
        neg = 1;
    }

    while(num != 0)
    {
        buf[i++] = digits[num % base];
        num = num / base;
    }

    if(neg)
    {
        buf[i] = '-';
    }
    else
    {
        i--;
    }
    
    for(; i >= 0; i--)
    {
        putc(buf[i]);
    }
}

void printf(const char *format, ...)
{
    spinlockLock(&cslLock);
    char *p = (char *)format;
    uint32_t *args = (uint32_t *)(void *)(&format + 1);
    while(*p)
    {
        if(*p == '%')
        {
            switch(*(++p))
            {
                case 'd':
                {

                    printi((int)*args, 10);
                    break;
                }
                case 'x':
                {
                    printi((int)*args, 16);
                    break;
                }
                case 's':
                {
                    puts((char *)(*args));
                    break;
                }
                case 'c':
                {
                    putc((char)*args);
                    break;
                }
                default:
                {
                    break;
                }
            }
            args++;
        }
        else
        {
            putc(*(char *)p);
        }
        p++;       
    }
    spinlockUnlock(&cslLock);
}

void cslTest()
{
    char text[] = "CONSOLE TEST";
    for(int i = 0; i < 10; i++)
    {
        printf("%s: %d\n", text, i);
    }
}

void cslInit()
{
    spinlockInit(&cslLock);
    spinlockLock(&cslLock);
    row = 0;
    col = 0;
    spinlockUnlock(&cslLock);
}