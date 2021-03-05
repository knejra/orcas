#ifndef _PROCESS_H
#define _PROCESS_H

#include "types.h"
#include "memory.h"
#include "console.h"
#include "concurrency.h"
#include "idt.h"
#include "fs.h"

#define PRCQUESIZE 64

// process state
#define P_UNUSED        0
#define P_RUNNABLE      1
#define P_RUNNING       2
#define P_SUSPEND       3
#define P_ZOMBIE        4
#define P_STOPPED       5

#define INIT_COUNTER     200

#define P_SCHED_RR      0
#define P_SCHED_FIFO    1

#define P_OFILE        10
#define P_NTHRS        10 

typedef uint32_t pid_t;

struct process
{
    pid_t pid;                        // process id
    uint32_t size;                    // memory size
    pde_t *mPgd;                      // page directory
    int status;                       // running state
    int priority;                     // priority
    struct thread *mThrs[P_NTHRS];    // threads
    struct file openFile[P_OFILE];    // openfile table
    void *cv;                        // condition variable
};

typedef uint32_t pid_t;

pid_t curpid;
struct process prcqueue[PRCQUESIZE];
struct process prc_scheduler;
spinlock_t prcLock;

// prcInit: initialize process manager
// parameters: void
// outputs   : void
void prcInit();

// forkret: excute after fork
// parameters: void
// ouputs    : void
void forkret();

// fork: create a new process
// parameters: void
// ouputs    : pid-process id
pid_t do_fork();

// schedule: process scheduler
// parameters: void
// outputs   : void
void schedule();

// yield: current process give up cpu, and call scheduler
// parameters: void
// outputs   : void
void yield();

// getCurrentProc: get current running process
// parameters: void
// outputs   : pointer refer to current process
struct process *getCurrentProc();

int forkTest();

void sleep();
void wakeup();
void kill(pid_t pid);
void waitpid(pid_t pid);
void exit();
void exec();

#endif /* _PROCESS_H */