#ifndef _FS_H
#define _FS_H

#include "types.h"
#include "ide.h"

// buffer cache
// the kernel attempts to minimize the frequency of disk access by keeping
// a pool of internal data buffers, called the buffer cache

#define BCACHE_SIZE   128 // 64KB
#define BLK_HASH_SIZE 128 // assume 64 block for each device

struct block_cache
{
    struct block buf[BCACHE_SIZE];
    struct block *lruListHead;
    struct block blkHash[BLK_HASH_SIZE];
};

struct block_cache bcache;

//#define BLKHASH(dev, blk) (((dev) << 3) | ((blk) * (SECSIZE / BSIZE)) % BLK_HASH_SIZE)
#define BLKHASH(dev, blk) ((blk) * (SECSIZE / BSIZE) % BLK_HASH_SIZE)

// blockCacheInit: initialize block cache
// parameters: void
// outputs   : void
void blockCacheInit();

// getCachedBlock: if block already in cache, return directly, otherwise
//                 get a free buffer from LRU list, mark it on hash
//                 and return it to user. Attention: the return buffer
//                 should be locked.
// parameters: device-device number
//             block-block number
// outputs:    block buffer
struct block *getCachedBlock(int device, int block);

// relCachedBlock: release a cached block
// parameters: blk-cached block ptr to release
// outputs   : void
void relCachedBlock(struct block *blk);

// blockRead: read a block into blk
// parameters: device-device number
//             block-block number
// outputs   : read state
struct block *blockRead(int device, int block);

// blockWrite: write a block to disk
// parameters: blk-block to write
// outputs   : write state
int blockWrite(struct block *blk);

// Test: buffer cache test
void blockCacheTest();

#define NDATA     64
#define IBUFSIZE  128
#define IHASHSIZE 128

#define I_NONE   0
#define I_FILE   1
#define I_DEV    2
#define I_PIPE   3
#define I_SOCK   4
#define I_DIR    5

#define I_NOTCACHE  0x0 // attention: 'unused' means not in cache!
#define I_LOCKED  0x1
#define I_VALID   0x2 // the cached inode is consistent with disk inode
#define I_DIRTY   0x3 // the cached inode has been modified


#define BSET(x, b) ((x) |= (1 << (b)))
#define BCLR(x, b) ((x) &= ~(1 << (b)))

// superblock: first block of device, contain metadata
struct superblock
{
    size_t size;          // total size
    int device;           // which device the superblock on
    int nLogBlock;        // num of log blocks
    int nInodeBlock;      // num of inode blocks
    int nDataBlock;       // num of data blocks
    uint32_t logStart;    // start of log blocks
    uint32_t inodeStart;  // start of inode blocks
    uint32_t bitmapStart; // start of bitmap blocks
    uint32_t dataStart;   // start of data blocks
};

// structure of disk inodes
struct diskInode
{
    uint16_t uid; // user id
    uint16_t gid; // group id
    uint16_t mode; // mode: rwx, r=0x1, w=0x2, x=0x4
    int type;     // type: file, directory, device, pipe
    int flag;
    uint32_t atime; // last time of access
    uint32_t ctime; // last time of inode change
    uint32_t mtime; // last time of modification
    uint16_t nlink; // link count
    uint32_t nblock; // number of data blocks
    uint32_t block[NDATA]; // address of data blocks
    
};

struct inode
{
    size_t size;              // file size
    int ino;                  // inode number
    int device;               // device number
    int access;               // access count
    struct diskInode dinode;  // disk inode
    struct inode *lru_prev;   // prev ptr for lru list
    struct inode *lru_next;   // next ptr for lru list
    struct inode *hash_prev;  // prev ptr for hash table
    struct inode *hash_next;  // next ptr for hash table
};

#define IPB (BSIZE / (sizeof(struct diskInode)))
#define IHASH(ino) ((ino) % (IHASHSIZE))

struct inode_cache
{
    struct inode ibuf[IBUFSIZE];
    struct inode *lruListHead;
    struct inode ihash[IHASHSIZE];
};

struct inode_cache icache;

// readSuperblock: read superblock of the device
// parameters: sb-keep retrun superblock in this param
//             device-device number
// outputs  :  read success or fail
int readSuperblock(struct superblock *sb, int device);

// writeSuperblock: write superblock to the device
// parameters: sb-superblock to write
//             device-device number
// outputs   : write success or fail
int writeSuperblock(struct superblock *sb, int device);

// diskLayoutInit: initialize disk layout
// parameters: void
// outputs   : void
void diskLayoutInit();

// Test: superblock test
void superblockTest();

// inodeCacheInit: initialize inode cache
// parameters: void
// outputs   : void
void inodeCacheInit();

// getCachedInode: get cached inode, if not in cache,
//                 return a free inode buffer
// parameters: ino-inode number
// outputs   : inode
struct inode *getCachedInode(int ino, int device);
// getInode: not used
struct inode *getInode(int ino, int device);

// putCachedInode: relase cached inode
// paramters: ind-inode to be released
// outputs  : void
int putCachedInode(int ino);

// bmap: block map of logical file byte offset to file system block
// parameters: ind-inode
//             offset-byte offset
// outputs   : block address
uint32_t bmap(struct inode *ind, uint32_t offset);

// allocInode: allocate a new inode
// parameters: device-device to allocate new disk inode
// outputs   : new inode
struct inode *allocInode(int device);

// freeInode: free a inode
// parameters: ino-inode number
// outputs: free success or not
int freeInode(int device, int ino);

// allocData: allocate a new data block
// parameters: device-device to allocate new data block
// outputs   : the address of new data block
uint32_t allocData(int device);

// freeData: free a data block
// parameters: addr-address to free
// output    : success or not
int freeData(int device, uint32_t addr);

// Test: inode test
void inodeTest();

// directory
#define NAMELEN 100

struct dentry
{
    int ino;
    char name[NAMELEN];
};

// getNextDir: get next subdir
// parameters: path-dir path
// outputs   : next subdir in path
char *getNextDir(char *path);

// namei: transfer a path into inode
// parameters: path-file path
// outputs   : inode the path refer to
struct inode *namei(char *path);

// void dirTest();

// open file table
#define NOPENFILE 128

#define M_NONE  0x0
#define M_READ  0x1
#define M_WRITE 0x2
#define M_EXEC  0x4

#define F_NONE 0
#define F_REG  1
#define F_DEV  2
#define F_PIPE 3
#define F_SOCK 4

struct file
{
    int type;      // regular, device, pipe, socket
    size_t off;    // read offset
    uint16_t flag; // 0000-0rwx-0rwx-0rwx
    struct inode *ind;
};

struct openFile
{
    int cnt; // number of process share this open file table
    int max_fd; // there maybe epoll someday
    uint32_t bitmap[4];
    //struct inode *fds[NOPENFILE];
    struct file fds[NOPENFILE];
};

// creat: crea a new file
// parameters: path-the path (and file name) of new file
//             mode-readable, writable, excutable
// outputs   : file descriptor
int creat(const char *path, int mode);

// read: read size of byte from file(fd) into buf
// parameters: fd-file descriptor
//             buf-buffer to read in
//             size-byte to read
// outputs  :  num of byte successfully read
int read(int fd, void *buf, size_t size);

// write: write size of byte of buf to file(fd)
// parameters: fd-file descriptor
//             buf-buffer to write
//             size-byte to write
// outputs   : num of byte successfully written
int write(int fd, void *buf, size_t size);

// open: open a file
// parameters: path-path of the file
//             mode-readable, writable, excutable
// outputs   : file descriptor
int open(const char *path, int mode);

// close: close a file
// parameters: fd-file descriptor
// outputs   : 0
int close(int fd);

// TODO: lseek
int lseek(size_t off);
// TODO: dup
int dup(int fd);

// ls: list files under current working directory
// parameters: void
// outputs   : success or not
int ls();

// mk: mkdir, make a new directory
// parameters: name-directory's name
// outputs   : success or not
int mk(char *name);

// cd: change directory
// parameters: path-path to change
// outputs: success or not
int cd(char *path);

// rm: remove file or directory
// parameter: path-file path
// outputs  : success or not
int rm(char *path);

// cat: print file content on screen
// parameters: path-file path
// output    : success or not
int cat(const char *path);

struct fs
{
    char cwd_name[NAMELEN];
    struct inode *cwd;
    struct inode *root;
    struct openFile ofTbl;
};

struct fs fileSys;

// fsInit: initialize file system
// parameters: void
// outputs   : void
void fsInit();
void fsLoad();

#endif // _FS_H