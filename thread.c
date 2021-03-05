#include "thread.h"
#include "memory.h"
#include "gdt.h"

int globalTestVar = 0;

extern void trapret();
extern void thread_entry();

void getCurStack(struct context **ctx)
{
    asm volatile("movl %%esp, %0" : "=r"(*ctx));
}

void printStack()
{
    uint32_t stack;
    asm volatile("movl %%esp, %0" : "=r"(stack));
    printf("stack: %d\n", stack);
}

void waitloops(int n)
{
    for(int i = 0; i < n; i++)
    {
        DO_NOTHING();
    }
}

void kernStub(void (*func)(void *), void *args)
{
    func(args);
    thrExit();
}

thread_t *thrAlloc()
{
    thread_t *t = NULL;
    for(int i = 0; i < THRQUESIZE; i++)
    {
        if(thrqueue[i].status == THR_UNUSED || thrqueue[i].status == THR_STOP)
        {
            t = &(thrqueue[i]);
            break;
        }
    }
    return t;
}

tid_t thrCreate(void (*thrFunc)(void *), void *args)
{
    thread_t *t = thrAlloc();
    if(t == NULL) return -1;

    char *sp = t->kstack + KSTACKSIZE;
    sp -= sizeof(struct trapframe);
    t->tf = (struct trapframe *)sp;
    
    sp -= sizeof(struct runtime);
    t->rt = (struct runtime *)sp;
    t->rt->eip = (uint32_t)trapret;
    t->rt->func = (uint32_t)thrFunc;
    t->rt->args = (uint32_t)args;

    sp -= sizeof(struct stub);
    t->st = (struct stub *)sp;
    t->st->eip = kernStub;

    sp -= sizeof(struct context);
    t->ctx = (struct context *)sp;
    memset(t->ctx, 0, sizeof(struct context));
    t->ctx->eip = thread_entry;

    t->status = THR_READY;
    t->counter = DEFAULT_COUNTER;

    return t->tid;
}

void thrInit()
{
    asm volatile("cli");
    spinlockInit(&thrque_lock);
    for(int i = 0; i < THRQUESIZE; i++)
    {
        thrqueue[i].tid = i;
        thrqueue[i].status = THR_UNUSED;
    }
    char *sp = thr_scheduler->kstack + KSTACKSIZE;
    sp -= sizeof(struct context);
    thr_scheduler->ctx = (struct context *)sp;
    memset(thr_scheduler->ctx, 0, sizeof(struct context));
    thr_scheduler->ctx->eip = thrSched;
    
    asm volatile("sti");
}

void thrSched()
{
    struct thread *t;
    for(;;)
    {
        int maxp = 0;
        for(int i = 0; i < THRQUESIZE; i++)
        {
            if(thrqueue[i].status == THR_READY && thrqueue[i].counter > maxp)
            {
                t = &(thrqueue[i]);
                maxp = thrqueue[i].counter;
            }
        }

        thr_current = t;
        thr_current->status = THR_RUNNING;
        ctxSwitch(&(thr_scheduler->ctx), thr_current->ctx);
    }
}

void thrYeild()
{
    thr_current->status = THR_READY;
    thr_current->counter--;
    ctxSwitch(&(thr_current->ctx), thr_scheduler->ctx);
}

thread_t *getCurThread()
{
    return thr_current;
}

tid_t thrGettid()
{
    return thr_current->tid; 
}

void thrJoin(tid_t tid)
{

}

void thrExit()
{
    thr_current->status = THR_ZOMBIE;
    ctxSwitch(&(thr_current->ctx), thr_scheduler->ctx);
}

void thrKill(tid_t tid)
{
    thrqueue[tid].status = THR_ZOMBIE;
}

void func(void *args)
{
    int i = 0;
    while(true)
    {
        if(i % 40000000 == 0)
        {
            printf("thread: %s\n", (char *)args);
        }
        i++;   
    }
    //printf("%s\n", (char *)args);
}

void thrTest()
{
    char ta[] = "A";
    tid_t t0 = thrCreate(func, ta);
    char tb[] = "B";
    tid_t t1 = thrCreate(func, tb);
    char tc[] = "C";
    tid_t t2 = thrCreate(func, tc);
    printf("thread A, B, C joined\n");
    char td[] = "D";
    tid_t t3 = thrCreate(func, td);
    printf("thread D joined\n");
    thrKill(t0);
}

void thrCondWait(void *cv, spinlock_t *spl)
{
    if(spl != &thrque_lock)
    {
        spinlockLock(&thrque_lock);
        spinlockUnlock(spl);
    }

    thread_t *t = getCurThread();
    t->cv = cv;
    t->status = THR_SLEEPING;
    thrSched();
    t->cv = (void *)NULL;

    if(spl != &thrque_lock)
    {
        spinlockUnlock(&thrque_lock);
        spinlockLock(spl);
    }
}

void thrCondSignal(void *cv)
{
    for(int i = 1; i < THRQUESIZE; i++)
    {
        if(thrqueue[i].status == THR_SLEEPING && thrqueue[i].cv == cv)
        {
            thrqueue[i].status = THR_READY;
            break;
        }
    }
}

void thrCondBroadcast(void *cv)
{
    for(int i = 1; i < THRQUESIZE; i++)
    {
        if(thrqueue[i].status == THR_SLEEPING && thrqueue[i].cv == cv)
        {
            thrqueue[i].status = THR_READY;
        }
    }
}

void gfunc0(void *arg)
{
    spinlockLock(&gtLock);
    // delay some times
    waitloops(1000000);
    globalTestVar += 10;
    spinlockUnlock(&gtLock);
    thrCondSignal(&globalTestVar);
    thrExit();
}

void gfunc1(void *arg)
{
    spinlockLock(&gtLock);
    while(globalTestVar == 23)
    {
        thrCondWait(&globalTestVar, &gtLock);
    }
    printf("gfunc: %d\n", globalTestVar);
    spinlockUnlock(&gtLock);
    thrExit();
}

void cvTest()
{
    spinlockInit(&gtLock);
    globalTestVar = 23;
    tid_t t0 = thrCreate(gfunc0, NULL);
    tid_t t1 = thrCreate(gfunc1, NULL);
}

void thrMutexInit(mutex_t *m)
{
    m->lock = 0;
    spinlockInit(&(m->lk));
}

void thrMutexLock(mutex_t *m)
{
    spinlockLock(&(m->lk));
    while(m->lock == 1)
    {
        thrCondWait(m, &(m->lk));
    }
    m->lock = 1;
    spinlockUnlock(&(m->lk));
}

void thrMutexUnlock(mutex_t *m)
{
    spinlockLock(&(m->lk));
    m->lock = 0;
    thrCondBroadcast(m);
    spinlockUnlock(&(m->lk));
}

void mfunc(void *arg)
{
    int local;
    for(int i = 0; i < 100; i++)
    {
        thrMutexLock(&gm);
        local = globalTestVar;
        local++;
        globalTestVar = local;
        printf("%d\n", globalTestVar);
        thrMutexUnlock(&gm);
    }
}

void mtxTest()
{
    thrMutexInit(&gm);
    globalTestVar = 23;
    tid_t t0 = thrCreate(mfunc, NULL);
    tid_t t1 = thrCreate(mfunc, NULL);
}

void thrSemInit(sem_t *s, int val)
{
    s->count = val;
    spinlockInit(&(s->lk));
}

void thrSemDown(sem_t *s)
{
    while(s->count <= 0)
    {
        getCurThread()->status = THR_SLEEPING;
        getCurThread()->cv = s;
    }
    s->count--;
}

void thrSemUp(sem_t *s)
{
    s->count++;
    for(int i = 0; i < THRQUESIZE; i++)
    {
        if(thrqueue[i].status == THR_SLEEPING && thrqueue[i].cv == s)
        {
            thrqueue[i].status = THR_READY;
            thrqueue[i].cv = NULL;
        }
    }
}

void producer(void *arg)
{
    for(int i = 0; i < 100; i++)
    {
        thrSemDown(&s_emp);
        slot[idx] = 1;
        printf("slot %d: full\n", idx++);
        thrSemUp(&s_full);
    }
    thrExit(); 
}

void consumer(void *arg)
{
    for(int i = 0; i < 100; i++)
    {
        thrSemDown(&s_full);
        slot[--idx] = 0;
        printf("slot %d: empty\n", idx);
        thrSemUp(&s_emp);
    }
    thrExit();
}

void semTest()
{
    idx = 0;
    thrSemInit(&s_emp, 5);
    thrSemInit(&s_full, 0);
    tid_t t0 = thrCreate(producer, NULL);
    tid_t t1 = thrCreate(consumer, NULL);
}

