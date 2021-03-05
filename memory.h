#ifndef _MEMORY_H
#define _MEMORY_H

#include "types.h"

// page table size
#define PGTSIZE 1024
// page directory size
#define PGDSIZE 1024
// page size
#define PGSIZE  4096

typedef uint32_t pde_t;
typedef uint32_t pte_t;
typedef uint32_t paddr_t;
typedef uint32_t vaddr_t;

// bits in page table entry
#define PAGE_PRESENT  0x1
#define PAGE_RW       0x2
#define PAGE_USER     0x4
#define PAGE_PWT      0x8
#define PAGE_PCD      0X10
#define PAGE_ACCESSED 0X20
#define PAGE_DIRTY    0x40
#define PAGE_PROTNONE 0x80

#define CPL_USER 0x0
#define CPL_KERN 0x3

// virtual mechine's memory may be less than 2GB
// when running in virtual mechaine, set kernel
// base as 0x8000000 (128MB), memory limit as
// 0x1fffffff (512MB), otherwise, set kernel base 
// as 0x80000000 (2GB), top as 0xffffffff (4GB)
#define VIRTUAL_MACHINE 0x1

#ifdef VIRTUAL_MACHINE

#define KERNELBASE 0x8000000
#define MEMLIMIT    0x1fffffff 

#else

#define KERNEL_BASE 0x80000000
#define MEMLIMIT    0xffffffff

#endif

// BIOS top
#define BIOSTOP 0x100000
#define KERNSIZE 0x10000000 // 256MB

// kernel virtual and physical memory map
#define VTOP(a) ((a) - KERNELBASE)
#define PTOV(a) ((a) + KERNELBASE)

// virtual memory address
// PAGE DIR IDX (10bits) | PAGE TBL IDX (10bits) | OFFSET (12bits)
#define PGDINDEX(va)  (((va) >> 22) & 0x3ff)
#define PGTINDEX(va)  (((va) >> 12) & 0x3ff)

// physical memory address and page table entry
// PAGE TBL PPN (20bits) | OFFSET (12bits)
#define PPN(pa)       ((pa) & ~(PGSIZE - 1))

// offset, 12 lower bits of physical and virtual address
#define OFFSET(a)     ((a) & 0xfff)

// page alignment
// upper bound of the page which address a belong to
#define PGUPBOUND(a)  (((a) + PGSIZE - 1) & ~(PGSIZE - 1))
// lower bound of the page which address a belong to
#define PGLOWBOUND(a) ((a) & ~(PGSIZE - 1))

pde_t *kpgdir;

struct page
{
    char* addr;
    struct page *next;
};

struct page pglist; // free page list
int pgavail; 

uint32_t brk; // pointer to kernel heap
uint32_t kmsize; // keep track of how many memory kernel allocate

// physical memory interface: stosb, memset,, lcr3
// stosb: set n continous cells which start at address as data
//        implement by assembly
// parameters: addr-start address
//             data-set value
//             cnt-count 
// outputs   : void
void stosb(void *addr, int data, int cnt);

// memset: a c wrap of stosb
// parameters: addr-start address
//             data-set value
//             cnt-count
// outputs   : void
void memset(void *addr, int data, int cnt);

// lcr3: load page directory into %cr3, implement by assembly
// parameters: val-load value
// outputs   : void
void lcr3(int val);

// lcr0: load value into %cr0, set to enable paging
// parameters: val-load value
// outputs   : void
void lcr0(int val);

// rcr0: read value from %cr0
// parameters: void
// outputs   : value of %cr0
int rcr0();

// physical page interface: allocpage, freepage
// allocPage: allocate a new physical page from kernel's heap
// parameters: void
// ouputs    : the start address of new allocated page
char *allocPage();

// freepage: free a physical page
// parameters: addr-address of physical page to be free
// outputs   : void
void freePage(void *addr);

// Test: allocate page test
void pageTest();

// physical and virtual address map functions: getpte, tranlate, mapaddr
// getpte: get virtual address va's pte
// paramters: pgd-current process's page directory 
//            va-virtual address
// outputs  : the page table entry of va
pte_t getpte(pde_t *pgd, vaddr_t va);

// translate: tanslate virtual address to physical address
// paramters: pgd-current process's page directory 
//            va-virtual address
// outputs  : physical address
paddr_t translate(pde_t *pgd, vaddr_t va);

// map: map physical address to virtual address, and return pte
// parameters: pgd-current process's page directory
//             pa-physical address
//             va-virtual address
//             flags-pte flags (12bits)
// outputs   : the page table entry
pte_t map(pde_t *pgd, paddr_t pa, vaddr_t va, uint16_t flags);

// rangemap: map pages in a certain range
// parameters: pgd-current process's page directory
//             pa-physical address
//             va-virtual address
//             flags-pte flags (12bits)
// outputs  : void
void rangemap(pde_t *pgd, paddr_t pa, vaddr_t va, size_t size, uint16_t flags);

// test: page mapping test
void mapTest();

// kmemInit: initialize kernel memory
// parameters: void
// outputs   : void
void kmemInit();

// switchpgt: switch process's page table
// parameters: npgd-new page directory
// outputs   : void
void switchpgt(pde_t *npgd);

// copypgt: copy process's page table, for child process
//          to copy parent's page table
// parameters: opgd-old page directory
// outputs   : npgd-new page directory
pde_t *copypgt(pde_t *opgd, size_t size);

// virtual page interface: vmalloc
// vmalloc: allocate new space
// parameters: pgd-current process's page directory
//             oldsize-old size of process
//             newsize-new size of process
//             privilege-kernel or user
// outputs  :  new space size
size_t vmalloc(pde_t *pgd, size_t oldsize, size_t newsize, int privilege);
// TODO: vmfree
void vmfree(pde_t *pgd, void *p);

// Test: malloc/free test
void mallocTest();

// pageFaultHandler: handle page fault
// parameters: void
// outputs   : void
void pageFaultHandler();

// Test: memory test
void memTest();

#endif // _MEMORY_H