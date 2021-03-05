#include "keyboard.h"
#include "port.h"
#include "console.h"
#include "util.h"
#include "fs.h"
#include "user.h"

extern int pos = 0;

void kbdDriverInit()
{
    while(inb(KBD_CMD_PORT) & KBD_DATA_INBUF)
    {
        inb(KBD_DATA_PORT);
    }
    // activate interrupts
    outb(KBD_CMD_PORT, 0xae);
    // get current state
    outb(KBD_CMD_PORT, 0x20);
    uint8_t state = (inb(KBD_DATA_PORT) | 1) & ~0x10;

    outb(KBD_CMD_PORT, 0x60);
    outb(KBD_DATA_PORT, state);
    outb(KBD_DATA_PORT, 0xf4);
}

int getc()
{
    uint8_t state, data;

    state = inb(KBD_CMD_PORT);
    if((state & KBD_DATA_INBUF) == 0)
    {
        return -1;
    }

    data = inb(KBD_DATA_PORT);
    return data;
}

int gets(char *str)
{
    char *p, c;
    p = str;
    while((c = getc()) != '\n')
    {
        *p = c;
        p++;
    }
    *p = '\0';
    return (p-str);
}

void parseCmd(char *line, char *cmd, char *arg)
{
    char *pl, *p;
    pl = line;
    p = cmd;

    // jump blank
    while(*pl == ' ' || *pl == '\0' || 
          *pl == '\b'|| *pl == '\t')
    {
        pl++;
    }

    while(*pl != '\0')
    {
        if(*pl == ' ')
        {
            *p = '\0';
            p = arg;
            pl++;
        }
        *p++ = *pl++;
    }

    *p = '\0';
}

void kbdInterruptHandler()
{
    int x = 0, y = 0;
    char key;
    if((key = getc()) != -1)
    {
        // break = make + 0x80
        if(!(key & 0x80))
        {
            //putc(keymap[key]);
            if((keymap[key] >= 'a' && keymap[key] <= 'z') || 
               (keymap[key] >= '0' && keymap[key] <= '9') ||
               (keymap[key] == ' '))
            {
                putc(keymap[key]);
                line[pos++] = keymap[key];
            }
            else if(keymap[key] == '\b')
            {
                getPos(&x, &y);
                setPos(x, y - 1, ' ');
                pos--;
            }
            else if(keymap[key] == '\n')
            {
                if(pos > 0)
                {
                    line[pos] = '\0';
                    parseCmd(line, cmd, arg);
                    if(!strcmp(cmd, "ls"))
                    {
                        ls();
                    }
                    else if(!strcmp(cmd, "cd"))
                    {
                        cd(arg);
                    }
                    else if(!strcmp(cmd, "mkdir"))
                    {
                        mk(arg);
                    }
                    else
                    {
                        printf("\n'%s' command not found", cmd);
                    }
                }
                pos = 0;
                printf("\narjenk@orcas: %s$ ", fileSys.cwd_name);
            }
        }
    }
}
