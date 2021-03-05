#ifndef _CONCURRENCY_H
#define _CONCURRENCY_H

#include "types.h"

struct spinlock
{
    uint32_t lock;
};

typedef struct spinlock spinlock_t;

// xchg: xchg in x86
// parameters: addr-memory to swap out
//             val-value to swap in
// outputs   : swap out result
uint32_t xchg(uint32_t *addr, uint32_t val);

// spinlockInit: initialize spinlock
// parameters: spl-spinlock
// outputs   : void
void spinlockInit(spinlock_t *spl);

// spinlockLock: lock spinlock
// parameters: spl-spinlock
// outputs   : void
void spinlockLock(spinlock_t *spl);

// spinlockUnlock: unlock spinlock
// paramters: spl-spinlock
// outputs  : void
void spinlockUnlock(spinlock_t *spl);

#endif // _CONCURRENCY_H