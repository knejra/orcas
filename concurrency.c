#include "concurrency.h"
#include "thread.h"

uint32_t xchg(uint32_t *addr, uint32_t val)
{
    uint32_t result;

    asm volatile("lock; xchg %0, %1" :
                 "+m" (*addr), "=a" (result) :
                 "1" (val) :
                 "cc");
    return result;
}

void spinlockInit(spinlock_t *spl)
{
    spl->lock = 0;
}

void spinlockLock(spinlock_t *spl)
{
    cli();
    while(xchg(&(spl->lock), 1) != 0);
}

void spinlockUnlock(spinlock_t *spl)
{
    spl->lock = 0;
    sti();
}
