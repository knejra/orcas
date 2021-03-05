#ifndef _PORT_H
#define _PORT_H

#include "types.h"

#define PIC_MASTER_CMD  0x20
#define PIC_MASTER_DATA 0x21
#define PIC_SLAVE_CMD   0xa0
#define PIC_SLAVE_DATA  0xa1

// port interface for byte: inb, outb
// inb: read a byte from port
// parameters: port-port
// outputs   : return readed byte
uint8_t inb(uint16_t port);

// outb: write a byte to port
// parameters: port-port, data-data
// outputs   : void
void outb(uint16_t port, uint8_t data);

// insl: read stream from port, for hard driver
// parameters: port-port
//             addr-destination address to write
//             cnt-count by 32bits
// outputs   : void
void insl(uint16_t port, void *addr, int cnt);

// outsl: write stream to port, for hard driver
// parameters: port-port
//             addr-source address for write
//             cnt-count by 32bits
// outpus    : void
void outsl(uint16_t port, void *addr, int cnt);

#endif // _PORT_H