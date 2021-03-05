#include "memory.h"
#include "console.h"
#include "process.h"
#include "util.h"
#include "fs.h"

extern char kernheap[];

void stosb(void *addr, int data, int cnt)
{
    asm volatile("cld; rep stosb" :
                "=D" (addr), "=c" (cnt) :
                "0" (addr), "1" (cnt), "a" (data) :
                "memory", "cc");
}

void memset(void *addr, int data, int cnt)
{
    stosb(addr, data, cnt);
}

void lcr3(int val)
{
    asm volatile("movl %0, %%cr3" : : "r" (val));
}

int rcr3()
{
    int data;
    asm volatile("movl %%cr3, %0" : "=r"(data));
    return data;
}

void lcr0(int val)
{
    asm volatile("movl %0, %%cr0" : : "a" (val));
}

int rcr0()
{
    int data;
    asm volatile("movl %%cr0, %0" : "=r"(data));
    return data;
}

void lcr4(int val)
{
    asm volatile("movl %0, %%cr4" : : "a"(val));
}

void switchPageTable(int pgd)
{
    lcr3(VTOP(pgd));
}

char *allocPage()
{
    char *addr;
    // if pglist is not empty, use pages
    // in pglist, else allocate new one
    // from kernel heap
    if(pgavail > 0)
    {
        addr = pglist.next->addr;
        pglist.next = pglist.next->next;
        pgavail--;
    }
    else
    {
        addr = brk;
        brk += PGSIZE;
    }
    return addr;
}

void freePage(void* addr)
{
    struct page *p;
    p->addr = (char *)addr;
    memset(p->addr, 0, PGSIZE);
    p->next = pglist.next;
    pglist.next = p;
    pgavail++;
}

void pageTest()
{
    char *paddr0 = allocPage();
    printf("page addr0: %x\n", (uint32_t)paddr0);
    char *paddr1 = allocPage();
    printf("page addr1: %x\n", (uint32_t)paddr1);
    char *paddr2 = allocPage();
    printf("page addr2: %x\n", (uint32_t)paddr2);
    freePage(paddr1);
    char *paddr3 = allocPage();
    printf("page addr3: %x\n", (uint32_t)paddr3);
}

pte_t getpte(pde_t *pgd, vaddr_t va)
{
    pte_t pte = 0;
    vaddr_t plb = PGLOWBOUND(va);
    pde_t pde = pgd[PGDINDEX(plb)];
    if(pde & PAGE_PRESENT)
    {
        pte_t *pgtbl = PPN(pde);
        if(pgtbl[PGTINDEX(plb)] & PAGE_PRESENT)
        {
            pte = pgtbl[PGTINDEX(plb)];
        }
    }

    return pte;
}

paddr_t translate(pde_t *pgd, vaddr_t va)
{
    paddr_t pa = 0;
    pte_t pte;
    if((pte = getpte(pgd, va)) != 0)
    {
        pa = PPN(pte) | OFFSET(va);
    }

    return pa;
}

pte_t map(pde_t *pgd, paddr_t pa, vaddr_t va, uint16_t flags)
{
    pte_t *pgtbl;

    // map address which belong to the same 
    // page to same lower bound of this page
    vaddr_t plb = PGLOWBOUND(va);
    //printf("lowerbound: %x\n", plb);
    pde_t pde = pgd[PGDINDEX(plb)];
    //printf("index: %d\n", PGDINDEX(plb));
    // if page table exists, read pte directly
    // otherwise, allocate a page for new page
    // table, write pte in new table
    if(pde & PAGE_PRESENT)
    {
        pgtbl = (pte_t *)(PPN(pde));
    }
    else
    {
        pgtbl = (pte_t *)allocPage();
        memset(pgtbl, 0, PGTSIZE * sizeof(pte_t));
        // attention: the address of page table change after map
        pde = (uint32_t)(pgtbl) | flags |PAGE_PRESENT;
    }
    
    //printf("ppn: %x\n", PPN(pa));
    pgd[PGDINDEX(plb)] = pde;
    pgtbl[PGTINDEX(plb)] = (pte_t)(PPN(pa) | flags | PAGE_PRESENT);

    //printf("pte: %x\n", pgtbl[PGDINDEX(plb)]);
    return pgtbl[PGTINDEX(plb)];
}

void rangemap(pde_t *pgd, paddr_t pa, vaddr_t va, size_t size, uint16_t flags)
{
    paddr_t plb = PGLOWBOUND(pa);
    paddr_t pub = plb + size;
    vaddr_t vlb = PGLOWBOUND(va);

    while(plb < pub)
    {
        map(pgd, plb, vlb, flags);
        plb += PGSIZE;
        vlb += PGSIZE;
    }
}

int pgenable()
{
    // enable paging
    lcr4(PGSIZE);
    switchPageTable(kpgdir);
    // unsigned int cr0 = rcr0();
    // cr0 |= 0x80000000;
    // lcr0(cr0);
}

void mapTest()
{
    char *pa0 = allocPage();
    vaddr_t va0 = PTOV(pa0);
    map(kpgdir, (paddr_t)pa0, va0, PAGE_RW);
    paddr_t ta0 = translate(kpgdir, va0);
    printf("paddr: %x, trans: %x\n", (uint32_t)pa0, (uint32_t)ta0);
    
    char *pa1 = allocPage();
    vaddr_t va1 = PTOV(pa1);
    map(kpgdir, (paddr_t)pa1, va1, PAGE_RW);
    paddr_t ta1 = translate(kpgdir, va1);
    printf("paddr: %x, trans: %x\n", (uint32_t)pa1, (uint32_t)ta1);

    char *pa2 = allocPage();
    rangemap(kpgdir, (paddr_t)pa2, PTOV(pa2), PGSIZE * 4, PAGE_RW);
    for(int i = 0; i < 4; i++)
    {
        paddr_t ta = translate(kpgdir, PTOV(pa2 + PGSIZE * i));
        printf("paddr: %x, trans: %x\n", (uint32_t)(pa2 + PGSIZE * i), ta);
    }
}

void kmemInit()
{
    pgavail = 0;
    brk = PGUPBOUND((uint32_t)kernheap);
    kmsize = brk;
    kpgdir = (pde_t *)allocPage();
    memset(kpgdir, 0, PGDSIZE * sizeof(pde_t));
    rangemap(kpgdir, 0, PTOV(0), KERNSIZE, PAGE_RW);
    //pgenable();
    switchPageTable(kpgdir);
}

void switchpgt(pde_t *npgd)
{
    lcr3(npgd);
}

pde_t *copypgt(pde_t *opgd, size_t size)
{
    pde_t *npgd = (pde_t *)allocPage();
    for(int p = 0; p < size; p += PGSIZE)
    {
        pte_t pte = getpte(opgd, (vaddr_t)p);
        uint16_t flags = OFFSET(pte);
        paddr_t pa = PPN(pte) | flags;
        char *npa = allocPage();
        memmove(npa, PTOV(pa), PGSIZE);
        map(npgd, p, VTOP(npa), flags);
    }

    return npgd;
}

size_t vmalloc(pde_t *pgd, size_t oldsize, size_t newsize, int privilege)
{
    int flags = PAGE_RW;
    if(privilege == CPL_USER)
    {
        flags |= PAGE_USER;
    }

    // allocate page once a time, if newsize still
    // in this page, change size to newsize and 
    // return, othersize allocate a newpage, map
    // to upbound (next page lowerbound)
    size_t upbound = PGUPBOUND(oldsize);
    while(upbound < newsize)
    {
        // allocated page address now is virtual
        char *pg = (char *)allocPage();
        // after lcr3, the operating address is virtual
        //map(pgd, VTOP(pg), pg, flags);
        upbound += PGSIZE;
    }

    return newsize;
}

void vmfree(pde_t *pgd, void *p)
{
    
}

void *sbrk(size_t size)
{
    uint32_t addr = kmsize;
    vmalloc(kpgdir, kmsize, kmsize + size, CPL_KERN);
    kmsize += size;
    return (void *)addr;
}

char *kmalloc(size_t size)
{
    return sbrk(size);
}

int kfree()
{

}

void mallocTest()
{
    printf("dentry: %x, inode: %x\n", sizeof(struct dentry), sizeof(struct inode));
    char *p1 = (char *)sbrk(sizeof(struct dentry));
    printf("sbrk: %x\n", (uint32_t)p1);
    char *p2 = (char *)sbrk(sizeof(struct inode));
    printf("sbrk: %x\n", (uint32_t)p2);
}

void pageFaultHandler()
{

}





