#include "ide.h"
#include "thread.h"

extern havedisk1 = 0;

int testIdeState()
{
    int state = inb(IDE_CMD_PORT);
    while((state & IDE_ST_BSY == IDE_ST_BSY) && 
          (state & IDE_ST_ERR != IDE_ST_ERR))
    {
        state = inb(IDE_CMD_PORT);
    }
    
    // write fault or error
    if((state & IDE_ST_DF == IDE_ST_DF) || 
       (state & IDE_ST_ERR == IDE_ST_ERR))
    {
        printi(state, 16);
        return -1;
    }

    return 0; 
}

int hardDriverInit()
{
    if(testIdeState() == -1)
    {
        printf("[Error] Hard Disk Error\n");
        return -1;
    }

    // check if disk1 exits
    // outb(IDE_DEVICE_PORT, 0xe0 | (1 << 4));
    // for(int i = 0; i < 1000; i++)
    // {
    //     if(inb(IDE_CMD_PORT) != 0)
    //     {
    //         havedisk1 = 1;
    //         break;
    //     }
    // }
    // outb(IDE_DEVICE_PORT, 0xe0 | (0 << 4));

    // if(havedisk1)
    // {
    //     printf("[harddisk] have disk %d\n", &havedisk1);
    // }

    outb(IDE_DEVICE_PORT, 0xa0);
    outb(IDE_CTL_PORT, 0);

    outb(IDE_DEVICE_PORT, 0xa0);
    uint8_t state = inb(IDE_CMD_PORT);
    if(state == 0xff)
    {
        printf("[Error] hard disk not exits\n");
        return -1;
    }

    outb(IDE_DEVICE_PORT, 0xa0);
    outb(IDE_SECNUM_PORT, 0);
    outb(IDE_SECNO0_PORT, 0);
    outb(IDE_SECNO8_PORT, 0);
    outb(IDE_SECNO16_PORT, 0);
    outb(IDE_CMD_PORT, 0xec); // identify

    state = inb(IDE_CMD_PORT);
    if(state == 0x00)
    {
        printf("[Error] hard disk inaccessible\n");
        return -1;
    }

    while((state & 0x80) == 0x80 &&
          (state & 0x01) != 0x01)
    {
        state = inb(IDE_CMD_PORT);
    }

    if(state & 0x01)
    {
        printf("[Error] hard disk error\n");
        return -1;
    }

    // initialize wait queue
    blkWaitQueue->lru_prev = blkWaitQueue;
    blkWaitQueue->lru_next = blkWaitQueue;

    //puts("[hard disk] hard driver runs\n");

    return 0;
}

void hardRead(struct block *blk)
{

    // if(testIdeState() == -1)
    // {
    //     return -1;
    // }

    // 1. generate interrupt
    outb(IDE_CTL_PORT, 0);

    // 2. write LBA
    int sector = (blk->block) * SEC_PER_BLK;
    outb(IDE_SECNUM_PORT, SEC_PER_BLK);
    outb(IDE_SECNO0_PORT, sector & 0xff);
    outb(IDE_SECNO8_PORT, (sector >> 8) & 0xff);
    outb(IDE_SECNO16_PORT, (sector >> 16) & 0xff);
    outb(IDE_DEVICE_PORT, 0xe0 | ((blk->device & 1) << 4) | ((sector >> 24) & 0x0f));

    // 3. add blk to tail of wait queue
    if(blkWaitQueue->lru_prev == blkWaitQueue)
    {
        blkWaitQueue->lru_next = blk;
        blkWaitQueue->lru_prev = blk;
        blk->lru_next = blkWaitQueue;
        blk->lru_prev = blkWaitQueue;
    }
    else
    {
        blk->lru_prev = blkWaitQueue->lru_prev;
        blkWaitQueue->lru_prev->lru_next = blk;
        blkWaitQueue->lru_prev = blk;
        blk->lru_next = blkWaitQueue;
    }

    // 4. tell ide to read
    outb(IDE_CMD_PORT, IDE_CMD_R_PIO);

    // 5. block current thread
    thrCondWait(blk, &thrque_lock);
}

void hardInterruptHandler()
{
    // 1. get front block from block wait queue
    // 2. read data from disk
    // 3. wake up threads who sleep on this block
}

int hardReadWait(struct block *blk)
{
    // if(testIdeState() == -1)
    // {
    //     return -1;
    // }

    // 1. generate interrupt
    outb(IDE_CTL_PORT, 0);

    // 2. write LBA
    int sector = (blk->block) * SEC_PER_BLK;
    outb(IDE_SECNUM_PORT, SEC_PER_BLK);
    outb(IDE_SECNO0_PORT, sector & 0xff);
    outb(IDE_SECNO8_PORT, (sector >> 8) & 0xff);
    outb(IDE_SECNO16_PORT, (sector >> 16) & 0xff);
    outb(IDE_DEVICE_PORT, 0xe0 | ((blk->device & 1) << 4) | ((sector >> 24) & 0x0f));

    // 3. tell ide to read
    outb(IDE_CMD_PORT, IDE_CMD_R_PIO);

    // 4. wait to read
    uint8_t state = inb(IDE_CMD_PORT);
    while ((state & IDE_ST_BSY == IDE_ST_BSY) 
           && (state & IDE_ST_ERR) != IDE_ST_ERR)
    {
        state = inb(IDE_CMD_PORT);
    }

    if(state & IDE_ST_ERR == IDE_ST_ERR)
    {
        printf("[Error] hard disk error\n");
        return -1;
    }

    // 5. read data from disk
    insl(IDE_DATA_PORT, blk->buf, BSIZE / 4);
    blk->flags = B_VALID;

    return 0;
}

int hardWrite(struct block *blk)
{

    // if(testIdeState() == -1)
    // {
    //     return -1;
    // }

    // 1. generate interrupt
    outb(IDE_CTL_PORT, 0);

    // 2. wirte LBA
    int sector = (blk->block) * SEC_PER_BLK;
    outb(IDE_SECNUM_PORT, BSIZE/SECSIZE);
    outb(IDE_SECNO0_PORT, sector & 0xff);
    outb(IDE_SECNO8_PORT, (sector >> 8) & 0xff);
    outb(IDE_SECNO16_PORT, (sector >> 16) & 0xff);
    outb(IDE_DEVICE_PORT, 0xe0 | ((blk->device & 1) << 4) | ((sector >> 24) & 0x0f));

    // 3. tell ide it's a pio write
    outb(IDE_CMD_PORT, IDE_CMD_W_PIO);

    // 4. write to disk
    outsl(IDE_DATA_PORT, blk->buf, BSIZE / 4);

    //outb(IDE_CMD_PORT, 0xe7);
    blk->flags = B_VALID;
    
    return 0;
}

void hardTest()
{
    struct block b;
    struct block rb;

    memset(b.buf, 0, BSIZE);
    char str[] = "the design of unix operating system";
    strncpy(b.buf, (const char *)str, strlen(str));
    b.device = 0;
    b.block = 1;
    b.flags = B_DIRTY;
    
    if(hardWrite(&b) != 0)
    {
        printf("[Error]: write failed\n");
        return ;
    }

    memset(rb.buf, 0, BSIZE);
    if(hardReadWait(&rb) != 0)
    {
        printf("[Error]: write failed\n");
        return ;
    }

    printf("[Hard Test] %s\n", rb.buf);
}
