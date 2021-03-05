#ifndef _THREAD_H
#define _THREAD_H

#include "types.h"
#include "console.h"
#include "idt.h"
#include "concurrency.h"

#define THRQUESIZE  256
#define KSTACKSIZE 1024

#define THR_UNUSED   0
#define THR_READY    1
#define THR_RUNNING  2
#define THR_SLEEPING 3
#define THR_ZOMBIE   4
#define THR_STOP     5

#define SCHED_RR    1  // Round Robin
#define SCHED_FIFO  2  // FIFO

#define DEFAULT_COUNTER  100 // init counter

#define DO_NOTHING() 

typedef uint32_t tid_t;

struct runtime
{
    uint32_t eip;
    uint32_t func;
    uint32_t args;
};

struct stub
{
    uint32_t eip;
};

struct context
{
    uint32_t edi;
    uint32_t esi;
    uint32_t ebx;
    uint32_t ebp;
    uint32_t eip;
};

struct thread
{
    tid_t tid;                  // thread id
    int status;                 // thread status
    int counter;                // left timer piece
    struct trapframe *tf;       // trapframe, for timer interrupt
    struct runtime *rt;         // runtime arguments
    struct stub *st;            // stub
    struct context *ctx;        // thread's context
    uint8_t kstack[KSTACKSIZE]; // kernel stack
    void *cv;                   // condition variable 
};

typedef struct thread thread_t;

thread_t *thr_scheduler, *thr_current;
spinlock_t thrque_lock;
thread_t thrqueue[THRQUESIZE];

// thread_entry: thread entry point, defined by switch.s
// parameters: void
// outputs   : void
void thread_entry();

// kernStub: to call thread function
// parameters: func-thread function
//             args-args of thread func
// outputs   : void
void kernStub(void (*func)(void *), void *args);

// ctxSwitch: switch context
// parameters: oldctx-context of current process
//             newctx-context of sheduling process
// outpus    : void
void ctxSwitch(struct context **oldctx, struct context *newctx);

// thrAlloc: allocate a new thread
// parameters: void
// outputs   : new thread
thread_t *thrAlloc();

// thrCreate: create a thread, like pthread_create
// parameters: thrFunc-the start function of this thread
//             args-argument of function
// ouputs    : return tid of new thread 
tid_t thrCreate(void thrFunc(void *), void *args);

// thrinit: initialize thread manager
// parameters: void
// outputs   : void
void thrInit();

// thrsched: schedule threads, round robin
// parameters: void
// outputs   : void
void thrSched();

// getCurThread: get current runnning thread
// parameters: void
// ouputs    : pointer to current thread
thread_t *getCurThread();

// thrGettid: get current thread's tid
// parameters: void
// outputs   : current thread's tid
tid_t thrGettid();

// thrYeild: current thread give up cpu, and call scheduler
// parameters: void
// outputs   : void
void thrYeild();

// thrJoin: wait thread(tid) to finish
// paramters: tid-thread's tid
// outputs  : void
void thrJoin(tid_t tid);

// thrExit: exit current thread
// parameters: void
// outputs   : void
void thrExit();

// thrKill: kill thread(tid)
// parameters: tid-thread's tid
// output    : void
void thrKill(tid_t tid);

void func(void *arg);
void thrTest();

// condition variable
// thrCondWait: wait on condition variable cv
// parameters: cv-condition variable
//             spl-spinlock
// outputs   : void
// attention: when use this function, the routine
//            should be:
//            spinlockLock(spl);
//            while(condition)
//              thrCondWait(cv, spl);
//            spinlockUnlock(spl);
void thrCondWait(void *cv, spinlock_t *spl);

// thrCondSignal: notify one thread waiting on cv
// paramters: cv-condition variable
// outputs  : void
void thrCondSignal(void *cv);

// thrCondBroadcast: notify all thread waiting on cv
// paramters: cv-condition variable
// outputs  : void
void thrCondBroadcast(void *cv);

int globalTestVar;
spinlock_t gtLock; // condition variable test lock
void gfunc0(void *arg);
void gfunc1(void *arg);
void cvTest();

// mutex
struct mutex
{
    uint32_t lock;
    spinlock_t lk;
};

typedef struct mutex mutex_t;

// thrMutexInit: initialize mutex
// parameters: m-mutex
// outputs  : void
void thrMutexInit(mutex_t *m);

// thrMutexLock: lock a mutex
// parameters: m-mutex
// outputs   : void
void thrMutexLock(mutex_t *m);

// thrMutexUnlock: unlock a mutex
// parameters: m-mutex
// outputs   : void
void thrMutexUnlock(mutex_t *m);

mutex_t gm;
void mfunc(void *arg);
void mtxTest();

// semaphore
struct semaphore
{
    int count;
    spinlock_t lk;
};

typedef struct semaphore sem_t;

// thrSemInit: initialize a semaphore
// parameters: s-semaphore
//             val-the init value
// outputs   : void
void thrSemInit(sem_t *s, int val);

// thrSemDown: the P operation
// parameters: s-semaphore
// outputs   : void
void thrSemDown(sem_t *s);

// thrSemUp: the V operation
// parameters: s-semaphore
// outputs   : void
void thrSemUp(sem_t *s);

sem_t s_emp, s_full;
int idx;
int slot[5];
void producer(void *arg);
void consumer(void *arg);
void semTest();

// TODO: read-write lock

#endif // _THREAD_H