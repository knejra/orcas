#include "fs.h"
#include "port.h"
#include "util.h"
#include "memory.h"
#include "process.h"
#include "thread.h"

void blockCacheInit()
{
    bcache.lruListHead->lru_prev = NULL;
    bcache.lruListHead->lru_next = NULL;

    // link all cache block to lru list
    for(int i = 0; i < BCACHE_SIZE; i++)
    {
        bcache.buf[i].device = -1;
        bcache.buf[i].block = -1;
        bcache.buf[i].flags = B_UNUSED;
        bcache.buf[i].lruCnt = 0;
        bcache.buf[i].lru_next = bcache.lruListHead->lru_next;
        if(bcache.lruListHead->lru_next != NULL)
        {
            bcache.lruListHead->lru_next->lru_prev = &(bcache.buf[i]);
        }
        bcache.buf[i].lru_prev = bcache.lruListHead;
        bcache.lruListHead->lru_next = &(bcache.buf[i]);
    }

    // link tail to the head to from a circle
    bcache.lruListHead->lru_prev = &(bcache.buf[0]);
    bcache.buf[0].lru_next = bcache.lruListHead;

    for(int i = 0; i < BLK_HASH_SIZE; i++)
    {
        bcache.blkHash[i].device = -1;
        bcache.blkHash[i].block = -1;
        bcache.blkHash[i].hash_next = NULL;
    }
}

struct block *getCachedBlock(int device, int block)
{
    struct block *blk;

    // if block already in cache, return the reference
    int key = BLKHASH(device, block);
    if(bcache.blkHash[key].hash_next != NULL)
    {
        blk = bcache.blkHash[key].hash_next;
        while(blk != NULL && blk->device != device &&
              blk->block != block)
        {
            blk = blk->hash_next;
        }

        if(blk->device == device && blk->block == block)
        {
            // remove block from LRU list
            blk->flags = B_VALID;
            blk->lru_prev->lru_next = blk->lru_next;
            blk->lru_next->lru_prev = blk->lru_prev;
            return blk;
        }
    }

    // block is not in cache, replace the least recently used block
    
    // no available buffer on LRU list, return error
    if(bcache.lruListHead->lru_next == bcache.lruListHead ||
       bcache.lruListHead->lru_next == NULL)
    {
        printf("[Error] buffer cache: no available buffer\n");
        return NULL;
    }

    blk = bcache.lruListHead->lru_next;
    bcache.lruListHead->lru_next = blk->lru_next;
    blk->lru_next->lru_prev = bcache.lruListHead;

    blk->device = device;
    blk->block = block;
    blk->lruCnt = 0;

    // add blk to hash
    blk->hash_next = bcache.blkHash[key].hash_next;
    blk->hash_prev = &(bcache.blkHash[key]);
    bcache.blkHash[key].hash_next = blk;

    return blk;
}

// attention: this function assume that blk is already in 
// cache, so it doesn't check if it really in cache. Must
// be careful when use it.
void relCachedBlock(struct block *blk)
{
    struct block *p;
    // remove from hash
    int key = BLKHASH(blk->device, blk->block);
    if(bcache.blkHash[key].hash_next != NULL)
    {
        p = bcache.blkHash[key].hash_next;
        while(p->hash_next != NULL && p->hash_next->device != blk->device &&
              p->hash_next->block != blk->block)
        {
            p = p->hash_next;
        }

        if(p->hash_next->device == blk->device && p->hash_next->block == blk->block)
        {
            p->hash_next = blk->hash_next;
            if(blk->hash_next != NULL)
            {
                blk->hash_next->hash_prev = p;
            }
        }
    }
    blk->hash_prev = NULL;
    blk->hash_next = NULL;
    blk->flags = B_UNUSED;

    // add to the tail of LRU list
    bcache.lruListHead->lru_prev->lru_next = blk;
    blk->lru_prev = bcache.lruListHead->lru_prev;
    bcache.lruListHead->lru_prev = blk;
    blk->lru_next = bcache.lruListHead;
}

struct block *blockRead(int device, int block)
{
    struct block *blk;
    blk = getCachedBlock(device, block);
    if(blk->flags == B_VALID)
    {
        return blk;
    }
    else
    {
        if(hardReadWait(blk) != 0)
        {
            printf("[Error] blockRead: hardReadWait\n");
            return NULL;
        }

        blk->flags = B_VALID;
    }

    return blk; 
}

int blockWrite(struct block *blk)
{
    struct block *cblk; 
    cblk = getCachedBlock(blk->device, blk->block);
    memmove(cblk->buf, blk->buf, BSIZE);
    return hardWrite(cblk);
}

void blockCacheTest()
{
    struct block b;

    char str[] = "Ernest Hermingway";
    strncpy(b.buf, (const char *)str, strlen(str));
    printf("[Block Write] write: %s\n", b.buf);

    b.device = 0;
    b.block = 2;
    b.flags = B_DIRTY;
    if(blockWrite(&b) != 0)
    {
        printf("[Error] blockCachedTest: blockWrite\n");
        return ;
    }

    struct block *rb;
    rb = blockRead(b.device, b.block);
    printf("[Block Read] first read: %s\n", rb->buf);

    rb = blockRead(b.device, b.block);
    printf("[Block Read] second read: %s\n", rb->buf);

    relCachedBlock(rb);
    struct block *tt = blockRead(b.device, b.block);
    printf("[Block Read] third read: %s\n", tt->buf);
}

int readSuperblock(struct superblock *sb, int device)
{
    struct block *blk;

    // superblock on the first block of device
    // notice: after read, the superblock will be keep in cache
    blk = blockRead(device, 1);
    if(blk == NULL)
    {
        printf("[Error] readSuperblock: blockRead\n");
        return -1;
    }

    memmove(sb, blk->buf, sizeof(struct superblock));
    return 0;
}

int writeSuperblock(struct superblock *sb, int device)
{
    struct block blk;

    blk.device = device;
    blk.block = 1;
    memset(blk.buf, 0, BSIZE);

    memmove(blk.buf, sb, sizeof(struct superblock));
    if(blockWrite(&blk) != 0)
    {
        printf("[Error] writeSuperblock: blockWrite\n");
        return -1;
    }

    return 0;
}

void diskLayoutInit()
{
    struct superblock sb;

    sb.nLogBlock = 10;
    sb.nInodeBlock = 10;
    sb.nDataBlock = 10;
    sb.logStart = 3;
    sb.inodeStart = 10;
    sb.bitmapStart = 20;
    sb.dataStart = 23;
    sb.device = 0;
    writeSuperblock(&sb, sb.device);
    // if(haveDisk1)
    // {
    //     sb.device = 1;
    //     writeSuperblock(&sb, sb.device);
    // }

    // set inode bitmap
    struct block bmap;
    bmap.device = sb.device;
    memset(bmap.buf, 0, BSIZE);
    bmap.block = sb.bitmapStart;
    blockWrite(&bmap);
    
    // set data bitmap
    bmap.block = sb.bitmapStart + 1;
    blockWrite(&bmap);
}

void superblockTest()
{
    struct superblock ws, rs;

    ws.device = 0;

    ws.nLogBlock = 30;
    ws.nInodeBlock = 32;
    ws.nDataBlock = 32;

    ws.logStart = 2;
    ws.inodeStart = 32;
    ws.dataStart = 64;

    writeSuperblock(&ws, 0);

    readSuperblock(&rs, 0);
    printf("[Log Start Block] %d\n", rs.logStart);
    printf("[Inode Start Block] %d\n", rs.inodeStart);
    printf("[Data Start Block] %d\n", rs.dataStart);
}

void inodeCacheInit()
{
    // 2020.10.15
    icache.lruListHead->lru_prev = NULL;
    icache.lruListHead->lru_next = NULL;

    for(int i = 0; i < IBUFSIZE; i++)
    {
        icache.ibuf[i].dinode.type = I_NOTCACHE;
        icache.ibuf[i].dinode.nlink = 0;
        icache.ibuf[i].size = 0;
        icache.ibuf[i].lru_next = icache.lruListHead->lru_next;
        if(icache.lruListHead->lru_next != NULL)
        {
            icache.lruListHead->lru_next->lru_prev = &(icache.ibuf[i]);
        }
        icache.ibuf[i].lru_prev = icache.lruListHead;
        icache.lruListHead->lru_next = &(icache.ibuf[i]);
    }

    icache.lruListHead->lru_prev = &(icache.ibuf[0]);
    icache.ibuf[0].lru_next = icache.lruListHead;

    for(int i = 0; i < IHASHSIZE; i++)
    {
        icache.ihash[i].ino = -1;
        icache.ihash[i].hash_next = NULL;
    }
}

struct inode *getCachedInode(int ino, int device)
{
    struct inode *ind = NULL;

    int key = IHASH(ino);
    // 1. if in cache, return directly
    if(icache.ihash[key].hash_next != NULL)
    {
        ind = icache.ihash[key].hash_next;
        while(ind != NULL && ind->ino != ino)
        {
            ind = ind->hash_next;
        }

        if(ind->ino == ino)
        {
            ind->dinode.flag = I_VALID;
            ind->access++;
            return ind;
        }
    }

    // 2. if not in cache, get a free inode first
    if(icache.lruListHead->lru_next == icache.lruListHead ||
       icache.lruListHead->lru_next == NULL)
    {
        printf("[Error] getCachedInode: no available buffer\n");
        return NULL;
    }

    ind = icache.lruListHead->lru_next;
    icache.lruListHead->lru_next = ind->lru_next;
    ind->lru_next->lru_prev = icache.lruListHead;

    ind->dinode.type = I_NOTCACHE;

    // // 3. read superblock
    // struct superblock sb;
    // readSuperblock(&sb, device);

    // // 4. calculate block and read it from disk
    // int iblk = (ino / IPB) + sb.inodeStart;
    // struct block *blk;
    // blk = blockRead(device, iblk);

    // // 5. calculate inode offset in block and
    // //    copy it to ind
    // int ioff = (ino % IPB) * sizeof(struct inode);
    // memmove(&(ind->dinode), blk->buf + ioff, sizeof(struct diskInode));

    // // // 6. release block
    // // relCachedBlock(blk);

    // 7. add to hash
    ind->hash_next = icache.ihash[key].hash_next;
    ind->hash_prev = &(icache.ihash[key]);
    icache.ihash[key].hash_next = ind;

    return ind;   
}

struct inode *getInode(int ino, int device)
{
    struct inode *ind;
    ind = getCachedInode(ino, device);
    if(ind->dinode.type == I_NOTCACHE)
    {
        struct superblock sb;
        struct block *ib;
        readSuperblock(&sb, ind->device);
    }
}

// int putCachedInode(int ino)
// {
//     return 0;
// }

// uint32_t bmap(struct inode *ind, uint32_t offset)
// {
//     int bn = offset / BSIZE;
//     if(ind->dinode.block[bn] == -1)
//     {
//         ind->dinode.block[bn] = allocData(ind->device);
//     }

//     return ind->dinode.block[bn];
// }

struct inode *allocInode(int device)
{
    struct superblock sb;
    struct block *bb;
    struct inode *ind;
    int ino = -1;

    // 1. read superblock
    readSuperblock(&sb, device);

    // 2. get inode bitmap, to find a free disk inode
    bb = blockRead(device, sb.bitmapStart);
    for(int i = 0; i < sb.nInodeBlock; i++)
    {
        if(bb->buf[i] == 0)
        {
            ino = i;
            break;
        }
    }
    bb->buf[ino] = 1;
    blockWrite(&bb);
    // notice
    // relCachedBlock(bb);

    if(ino == -1)
    {
        printf("[Error] allocInode: no available inode block\n");
        return NULL;
    }

    // 3. getCachedInode
    ind = getCachedInode(ino, device);
    ind->ino = ino;
    ind->device = device;
    ind->access = 0;
    ind->size = 0;

    // 4. initialize disk inode
    memset(&(ind->dinode), 0, sizeof(struct diskInode));

    // 5. write cached inode to disk注意大小！
    struct block *ib;
    ib = blockRead(device, (ino / IPB) + sb.inodeStart);
    int off = (ino % IPB) * sizeof(struct diskInode);
    memmove(ib->buf + off, &(ind->dinode), sizeof(struct diskInode));
    blockWrite(ib);
    // relCachedBLock(ib);
    
    return ind;
}

int freeInode(int device, int ino)
{
    struct superblock sb;
    struct block *bb;

    // 1. read superblock
    readSuperblock(&sb, device);

    // 2. read inode bitmap
    bb = blockRead(device, sb.bitmapStart);

    // 3. mark bitmap[ino] as 0
    if(ino > sb.nInodeBlock)
    {
        printf("[Error] freeInode: invalid inode number\n");
        return -1;
    }

    bb->buf[ino] = 0;
    blockWrite(bb);

    // 4. free cached block
    // relCachedBlock(bb);

    return 0;
}

void inodeTest()
{
    struct inode *ind = allocInode(0);
    if(ind == NULL)
    {
        printf("[Error] inodeTest: allocInode\n");
        return ;
    }

    printf("[Inode Test] ino: %d\n", ind->ino);

    ind = allocInode(0);
    if(ind == NULL)
    {
        printf("[Error] inodeTest: allocInode\n");
        return ;
    }

    printf("[Inode Test] ino: %d\n", ind->ino);

}

uint32_t allocData(int device)
{
    struct superblock sb;
    struct block *db;
    uint32_t addr = -1;

    // 1. read superblock
    readSuperblock(&sb, device);

    // 2. get data bitmap
    db = blockRead(device, sb.bitmapStart + 1);
    for(int i = 0; i < sb.nDataBlock; i++)
    {
        if(db->buf[i] == 0)
        {
            addr = i;
            break;
        }
    }
    db->buf[addr] = 1;
    blockWrite(db);
    // notice
    relCachedBlock(&db);

    // 3. clear disk
    // memset(db->buf, 0, BSIZE);
    // hardWrite(db);

    return sb.dataStart + addr;
}

int freeData(int device, uint32_t addr)
{
    struct superblock sb;
    struct block *db;

    readSuperblock(&sb, device);

    db = blockRead(device, sb.bitmapStart + 1);
    if(addr > sb.nDataBlock)
    {
        printf("[Error] freeData: invalid data block number\n");
        return -1;
    }

    db->buf[addr] = 0;
    blockWrite(&db);

    relCachedBlock(db);

    return 0;
}

void dataTest()
{
    uint32_t addr0 = allocData(0);
    printf("[Data Test] addr0: %d\n", addr0);
    uint32_t addr1 = allocData(0);
    printf("[Data Test] addr1: %d\n", addr1);
    freeData(0, addr0);
    uint32_t addr2 = allocData(0);
    printf("[Data Test] addr2: %d\n", addr2);
    
    uint32_t addr[10];
    for(int i = 0; i < 10; i++)
    {
        addr[i] = allocData(0);
        printf("[Data Test] addr: %d\n", addr[i]);
    }
}

// need check
char *getNextDir(char *path)
{
    char *p, *q;

    if(*path == '/')
    {
        p = path + 1;
    }
    else
    {
        p = path;
    }

    q = p;
    
    while(*p != '\0' && *p != '/')
    {
        p++;
    }

    *p = '\0';

    return q;
}

struct inode *namei(char *path)
{
    struct inode *ind;
    struct dentry de;

    // 1. get current inode
    if(*path == '/')
    {
        ind = fileSys.root;
    }
    else
    {
        ind = fileSys.cwd;
    }

    // replace with kmalloc!
    char terms[NTERM][LTERM];
    int nterm = split(path, '/', terms);

    for(int i = 0; i < nterm; i++)
    {
        struct block *db;
        for(int i = 0; i < (ind->size) / BSIZE; i++)
        {
            db = blockRead(ind->device, ind->dinode.block[i]);
            for(int off = 0; off < BSIZE; off += sizeof(struct dentry))
            {
                memmove(&de, db->buf + off, sizeof(struct dentry));
                if(!strcmp(de.name, terms[i]))
                {
                    ind = getCachedInode(de.ino, ind->device);
                    if(ind->dinode.type == I_NOTCACHE)
                    {
                        struct superblock sb;
                        struct block *ib;
                        readSuperblock(&sb, ind->device);
                        ib = blockRead(ind->device, (de.ino / IPB) + sb.inodeStart);
                        int off = (de.ino % IPB) * sizeof(struct diskInode);
                        memmove(&(ind->dinode), ib->buf + off, sizeof(struct diskInode));
                        // relCachedBlock(ib);
                    }
                }
            }
        }
        
        db = blockRead(ind->device, ind->dinode.block[ind->size / BSIZE]);
        for(int off = 0; off < (ind->size) % BSIZE; off += sizeof(struct dentry))
        {
            memmove(&de, db->buf + off, sizeof(struct dentry));
            if(!strcmp(de.name, terms[i]))
            {
                ind = getCachedInode(de.ino, ind->device);
                if(ind->dinode.type == I_NOTCACHE)
                {
                    struct superblock sb;
                    struct block *ib;
                    readSuperblock(&sb, ind->device);
                    ib = blockRead(ind->device, (de.ino / IPB) + sb.inodeStart);
                    int off = (de.ino % IPB) * sizeof(struct diskInode);
                    memmove(&(ind->dinode), ib->buf + off, sizeof(struct diskInode));
                    // relCachedBlock(ib);
                }
            }
        }
    }

    return ind;
}

// devide path into parent name and file name
// return file name, and out param pname
char *getFileName(const char *path, char *pname)
{
    char *p1 = path, *p2 = path;
    while(*p1 != '\0')
    {
        if(*p1 == '/')
        {
            p2 = p1;
        }
        *pname = *p1;
        p1++;
    }
    *pname = *p1;
    return p2++;
}

int creat(const char *path, int mode)
{
    struct inode *ind;
    struct dentry de;
    int fd;

    // 1. allocate inode
    ind = allocInode(fileSys.cwd->device);
    if(ind == NULL)
    {
        printf("[Error] creat: allocInode\n");
        return -1;
    }
    ind->dinode.type = I_FILE;

    // 2. create dentry, pack to parent directory's inode
    //    if path is a file name, pack dentry to current
    //    work directory, otherwise, use namei to find its
    //    parent inode
    char pname[NAMELEN];
    struct inode *pind;
    de.ino = ind->ino;
    memset(de.name, 0, NAMELEN);
    if(*path == '/')
    {
        char *name = getFileName(path, pname);
        strncpy(de.name, name, strlen(name));
        pind = namei(pname);
    }
    else
    {
        strncpy(de.name, path, strlen((char *)path));
        pind = fileSys.cwd;
    }

    int iblk = pind->dinode.block[(pind->size) / BSIZE];
    int off = (pind->size) % BSIZE;
    pind->size += sizeof(struct dentry);
    struct block *db = blockRead(pind->device, iblk);
    if(db == NULL)
    {
        printf("[Error] creat: blockRead\n");
        return -1;
    }
    memmove(db->buf + off, &de, sizeof(struct dentry));
    if(blockWrite(db) != 0)
    {
        printf("[Error] creat: blockWrite\n");
        return -1;
    }

    // 4. allocate one data block, adjust when write 
    ind->dinode.block[0] = allocData(ind->device);

    // 5. add ind to open file table
    fd = fileSys.ofTbl.max_fd++;
    fileSys.ofTbl.fds[fd].ind = ind;
    fileSys.ofTbl.fds[fd].flag = mode;
    fileSys.ofTbl.fds[fd].off = 0;
    
    return fd;
}

int ls()
{
    struct inode *ind;
    ind = fileSys.cwd;

    struct block *db;
    struct dentry de;
    for(int i = 0; i < (ind->size) / BSIZE; i++)
    {
        db = blockRead(ind->device, ind->dinode.block[i]);
        if(db == NULL)
        {
            printf("[Error] ls: blockRead\n");
            return -1;
        }
        for(int off = 0; off < BSIZE; off += sizeof(struct dentry))
        {
            memmove(&de, db->buf + off, sizeof(struct dentry));
            printf("%s\n", de.name);
        }
    }

    db = blockRead(ind->device, ind->dinode.block[(ind->size) / BSIZE]);
    if(db == NULL)
    {
        printf("[Error] ls: blockRead\n");
        return -1;
    }
    printf("\n");
    for(int off = 0; off < (ind->size) % BSIZE; off += sizeof(struct dentry))
    {
        memmove(&de, db->buf + off, sizeof(struct dentry));
        printf("%s ", de.name);
    }
    //puts("\n");

    return 0;
}

int mk(char *name)
{
    struct inode *ind;
    struct dentry de;

    ind = allocInode(fileSys.cwd->device);
    if(ind == NULL)
    {
        printf("[Error] mk: allocInode\n");
        return -1;
    }
    ind->dinode.type = I_DIR;

    char pname[NAMELEN];
    struct inode *pind;
    de.ino = ind->ino;
    memset(de.name, 0, NAMELEN);

    strncpy(de.name, name, strlen(name));
    pind = fileSys.cwd;

    int iblk = pind->dinode.block[pind->size / BSIZE];
    int off = (pind->size) % BSIZE;
    pind->size += sizeof(struct dentry);
    struct block *db = blockRead(pind->device, iblk);
    if(db == NULL)
    {
        printf("[Error] mk: blockRead\n");
        return -1;
    }
    memmove(db->buf + off, &de, sizeof(struct dentry));
    if(blockWrite(db) != 0)
    {
        printf("[Error] mk: blockWrite\n");
        return -1;
    }

    ind->dinode.block[0] = allocData(ind->device);

    return 0;
}

int pathcat(char *parent, char *child)
{
    strcat(parent, "/");
    strcat(parent, child);
}

int cd(char *path)
{
    struct inode *ind = namei(path);
    if(ind->dinode.type != I_DIR)
    {
        printf("[Error] %s is not a directory\n", path);
        return -1;
    }

    fileSys.cwd = ind;
    //strncpy(fileSys.cwd_name, path, strlen(path));
    pathcat(fileSys.cwd_name, path);
    return 0;
}

int rm(char *path)
{
    char pname[NAMELEN];
    char *name;
    struct inode *pind, *ind;
    if(*path == '/')
    {
        name = getFileName(path, pname);
        pind = namei(pname);
    }
    else
    {
        name = path;
        pind = fileSys.cwd;
    }
    
    // clear dentry
    struct block *db;
    struct dentry de;
    for(int i = 0; i < pind->size / BSIZE; i++)
    {
        db = blockRead(pind->device, pind->dinode.block[i]);
        if(db == NULL)
        {
            printf("[Error] rm: blockRead\n");
            return -1;
        }
        for(int off = 0; off < BSIZE; off += sizeof(struct dentry))
        {
            memmove(&de, db->buf + off, sizeof(struct dentry));
            if(!strcmp(de.name, name))
            {
                memset(db->buf + off, 0, sizeof(struct dentry));
                goto found;
            }
        }
    }

    db = blockRead(pind->device, pind->dinode.block[pind->size / BSIZE]);
    if(db == NULL)
    {
        printf("[Error] rm: blockRead\n");
        return -1;
    }
    for(int off = 0; off < pind->size % BSIZE; off += sizeof(struct dentry))
    {
        memmove(&de, db->buf + off, sizeof(struct dentry));
        if(!strcmp(de.name, name))
        {
            memset(db->buf + off, 0, sizeof(struct dentry));
            goto found;
        }
    }

    found:
    ind = getCachedInode(de.ino, pind->device);
    if(ind->dinode.type == I_NOTCACHE)
    {
        struct superblock sb;
        struct block *ib;
        readSuperblock(&sb, ind->device);
        ib = blockRead(ind->device, (de.ino / IPB) + sb.inodeStart);
        int off = (de.ino % IPB) * sizeof(struct diskInode);
        memmove(&(ind->dinode), ib->buf + off, sizeof(struct diskInode));
        // relCachedBlock(ib);
    }
    // free data
    for(int i = 0; i <= ind->size / BSIZE; i++)
    {
        freeData(ind->device, ind->dinode.block[i]);
    }
    // free inode
    freeInode(ind->device, ind->ino);
    
    return 0;
}

int read(int fd, void *buf, size_t size)
{
    char *pb = (char *)buf;
    struct block *db;

    struct inode *ind = fileSys.ofTbl.fds[fd].ind;
    if(ind == NULL)
    {
        return -1;
    }

    // read size byte from start
    int start = fileSys.ofTbl.fds[fd].off;
    int cur = start / BSIZE;
    int off = start % BSIZE;
    int nr = 0;
    if(size > BSIZE - off)
    {
        nr = (size - (BSIZE - off)) / BSIZE + 1;
    }
    if(start + size > ind->size)
    {
        printf("[Error] read: no enough data\n");
        return -1;
    }

    db = blockRead(0, ind->dinode.block[cur]);
    if(size > BSIZE - off)
    {
        memmove(pb, db->buf + off, BSIZE - off);
    }
    else
    {
        memmove(pb, db->buf + off, size);
    }
    pb += (size > BSIZE - off ? BSIZE - off : size);
    for(int i = 1; i < nr; i ++)
    {
        db = blockRead(0, ind->dinode.block[cur + i]);
        memmove(pb, db->buf, BSIZE);
        pb += BSIZE;
    }

    if(nr >= 1)
    {
        db = blockRead(0, ind->dinode.block[cur + nr]);
        memmove(pb, db->buf, (size - (BSIZE - off)) % BSIZE);
        pb += (size - (BSIZE - off) % BSIZE);
    }

    fileSys.ofTbl.fds[fd].off += size;
    return size;
}

int write(int fd, void *buf, size_t size)
{
    char *pb = (char *)buf;
    struct block *db;

    struct inode *ind = fileSys.ofTbl.fds[fd].ind;
    if(ind == NULL)
    {
        return -1;
    }

    // current block number and block to allocate
    int cur = ind->size / BSIZE;
    int off = ind->size % BSIZE;
    int na = 0;
    if(size > BSIZE - off)
    {
        na = (size - (BSIZE - off)) / BSIZE + 1;
    }

    if(cur + na >= NDATA)
    {
        printf("[Error] write: no enough data block\n");
        return -1;
    }
    // allocate new data block
    for(int i = 1; i <= na; i++)
    {
        ind->dinode.block[cur + i] = allocData(0);
    }

    // fill blocks
    db = blockRead(0, ind->dinode.block[cur]);
    if(size > BSIZE - off)
    {
        memmove(db->buf + off, pb, BSIZE - off);
    }
    else
    {
        memmove(db->buf + off, pb, size);
    }
    blockWrite(db);
    pb += (size > BSIZE - off ? BSIZE - off : size);
    for(int i = 1; i < na; i++)
    {
        db = blockRead(0, ind->dinode.block[cur + i]);
        memmove(db->buf, pb, BSIZE);
        pb += BSIZE;
        blockWrite(db);
    }

    if(na >= 1)
    {
        db = blockRead(0, ind->dinode.block[cur + na]);
        memmove(db->buf, pb, (size - (BSIZE - off)) % BSIZE);
        pb += (size - (BSIZE - off)) % BSIZE;
        blockWrite(db);
    }

    // adjust inode's size
    // in fact write may be failed, the return size 
    // should be size already written
    ind->size += size;
    return size;
}

int open(const char *path, int mode)
{
    int fd = -1;
    struct inode *ind = namei(path);
    if(ind == NULL)
    {
        fd = creat(path, mode);
    }
    else
    {
        for(int i = 0; i < 4; i++)
        {
            for(int j = 0; j < 32; j++)
            {
                int bit = (fileSys.ofTbl.bitmap[i] >> j) & 0x01;
                if(bit == 0)
                {
                    fd = i * 4 + j;
                    break;
                }
            }
        }

        if(fd > fileSys.ofTbl.max_fd)
        {
            fileSys.ofTbl.max_fd = fd;
        }
        fileSys.ofTbl.fds[fd].ind = ind;
        fileSys.ofTbl.fds[fd].off = 0;
        // naive code: inode and file should be same type
        fileSys.ofTbl.fds[fd].type = ind->dinode.type;
        fileSys.ofTbl.fds[fd].flag = mode;
    }

    return fd;
}

int close(int fd)
{
    fileSys.ofTbl.fds[fd].ind = NULL;
    fileSys.ofTbl.fds[fd].off = 0;
    fileSys.ofTbl.fds[fd].type = F_NONE;
    fileSys.ofTbl.fds[fd].flag = M_NONE;
    // clear bitmap
    BCLR(fileSys.ofTbl.bitmap[fd / 4], fd % 4);
}

int eof(int fd)
{
    struct inode *ind = fileSys.ofTbl.fds[fd].ind;
    return (fileSys.ofTbl.fds[fd].off < ind->size ? 0 : 1); 
}

int cat(const char *path)
{
    int fd = open(path, M_READ);
    char buffer[BSIZE];
    size_t size = fileSys.ofTbl.fds[fd].ind->size;
    for(int i = 0; i < size / BSIZE; i++)
    {
        read(fd, buffer, BSIZE);
        printf("%s", buffer);
    }
    read(fd, buffer, size % BSIZE);
    printf("%s", buffer);
    close(fd);
    
    return 0;
}

void fileTest()
{
    // create and mkdir
    int fd0 = creat("orcas0", M_READ);
    //puts("[Test] creat\n");
    //printf("fd %d ", &fd0);
    int fd1 = creat("orcas1", M_READ);
    //printf("%d ", &fd1);
    int fd2 = creat("orcas2", M_READ);
    //printf("%d\n", &fd2);
    char dir[] = "orcas3";
    mk(dir);

    // char name[] = "orcas1";
    // struct inode *ind = namei(name);
    // printf("namei: %d\n", &(ind->ino));
    // printf("oftbl: %d\n", &(fileSys.ofTbl.fds[fd1].ind->ino));

    // ls
    printf("[Test] ls\n");
    ls();
    printf("\n");

    // TODO: cd, rm

    // write
    printf("[Test] write\n");
    char text[] = "Harry Potter and the Goblet of Fire";
    size_t size = strlen(text);
    write(fd0, text, size);
    printf("%s\n", text);
    // read
    printf("[Test] read\n");
    char buffer[40];
    memset(buffer, 0, sizeof(buffer));
    read(fd0, buffer, size);
    printf("%s\n", buffer);

    // TODO: cat
}

void fsInit()
{
    // 1. hard driver
    hardDriverInit();

    // 2. caches
    blockCacheInit();
    inodeCacheInit();
    // 3. disk layout
    diskLayoutInit();

    // initialize root
    fileSys.root = allocInode(0);
    (fileSys.root)->dinode.block[0] = allocData(0);
    fileSys.cwd = fileSys.root;
    char rname[] = "root/";
    strncpy(fileSys.cwd_name, rname, strlen(rname));
    fileSys.ofTbl.cnt = 0;
    fileSys.ofTbl.max_fd = 0;

    //hardTest();
    //superblockTest();
    //blockCacheTest();
    //dataTest();
    //inodeTest();
    fileTest();
}

void fsLoad()
{
    hardDriverInit();
    blockCacheInit();
    inodeCacheInit();
    // superblock has been on disk
    // load root inode
}
