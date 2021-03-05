#ifndef _IDE_H
#define _IDE_H

#include "util.h"
#include "concurrency.h"

#define IDE_DATA_PORT    0x1f0 
#define IDE_SECNUM_PORT  0x1f2
#define IDE_SECNO0_PORT  0x1f3 // 0-7 bits of sector number
#define IDE_SECNO8_PORT  0x1f4 // 8-15 bits of sector number
#define IDE_SECNO16_PORT 0x1f5 // 16-23 bits of sector number
#define IDE_DEVICE_PORT 0x1f6 // 24-28 bits of sector number
#define IDE_CMD_PORT     0x1f7

#define IDE_CTL_PORT     0x3f6

// status
#define IDE_ST_ERR  0x01 // error
#define IDE_ST_IDX  0x02 // index
#define IDE_ST_CORR 0x04 // corrected data
#define IDE_ST_DRQ  0x08 // data request ready
#define IDE_ST_DSC  0x10 // drive seek complete
#define IDE_ST_DF   0x20 // drive wirte fault
#define IDE_ST_DRDY 0x40 // drive ready
#define IDE_ST_BSY  0x80 // busy

// errors
#define IDE_ER_AMNF  0x01 // address not found
#define IDE_ER_TK0NF 0x02 // track 0 not found
#define IDE_ER_ABRT  0x04 // command aborted
#define IDE_ER_MCR   0x08 // media change request
#define IDE_ER_IDNF  0x10 // ID mark not found
#define IDE_ER_MC    0x20 // media changed
#define IDE_ER_UNC   0x40 // uncorrecteable data
#define IDE_ER_BBK   0x80 // bad block

// port
#define IDE_CMD_R_PIO  0x20 // pio read
#define IDE_CMD_W_PIO  0x30 // pio write
#define IDE_CMD_MR_PIO 0xc4 // pio multiple read
#define IDE_CMD_MW_PIO 0xc5 // pio multiple write
#define IDE_CMD_R_DMA  0xc8 // DMA read
#define IDE_CMD_W_DMA  0xca // DMA write

#define SECSIZE     512 // sector size
#define BSIZE       512 // default: block size = sector size = 512 bytes
#define SEC_PER_BLK (BSIZE / SECSIZE)

#define B_UNUSED 0x1 // the block has been allocated but not used
#define B_VALID  0x2 // the block in-core is same with the copy on disk
#define B_DIRTY  0x4 // the block in-core is different with the copy on disk

// operating system are read from disk0, so
// disk is exist in default. operating system
// should check if the disk1 exits
int haveDisk1;

struct block
{
    int flags;       // F_READ, F_WRITE
    int device;      // device number
    int block;       // block number
    int lruCnt;      // count of recent use
    char buf[BSIZE]; // date buffer
    struct block *lru_prev;
    struct block *lru_next;
    struct block *hash_prev;
    struct block *hash_next;
    spinlock_t lock;
};

// block wait queue
struct block *blkWaitQueue;

// testIdetState: check if the ide is ready
// parameters: void
// outputs   : success or fail
int testIdeState();

// hardDriverInit: initialize harddisk driver
// parameters: void
// outputs   : success or fail
int hardDriverInit();

// hardRead: read from disk
// parameters: blk-read into this block
// outputs   : void
void hardRead(struct block *blk);

// hardInterruptHandler: hard intterurpt handler
// parameters: void
// outputs   : void
void hardInterruptHandler();

// hardReadWait: hard read (directly, not interrupt)
// parameters: blk-read into this block
// outputs   : success or not
int hardReadWait(struct block *blk);

// hardWrite: write to disk
// parameters: blk-write this block to disk
// outputs   : success or not
int hardWrite(struct block *blk);

// Test: hard disk driver test
void hardTest();

#endif // _IDE_H