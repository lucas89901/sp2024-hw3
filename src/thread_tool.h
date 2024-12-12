#ifndef THREAD_TOOL_H
#define THREAD_TOOL_H

#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>

// The maximum number of threads.
#define THREAD_MAX 100


void sighandler(int signum);
void scheduler();

#define SCHEDULER_JUMP_YIELD 1
#define SCHEDULER_JUMP_EXIT 2

// The thread control block structure.
struct tcb {
    int id;
    int *args;
    // Reveals what resource the thread is waiting for. The values are:
    //  - 0: no resource.
    //  - 1: read lock.
    //  - 2: write lock.
    int waiting_for;
    int sleeping_time;
    jmp_buf env;  // Where the scheduler should jump to.
    int n, i, f_cur, f_prev; // TODO: Add some variables you wish to keep between switches.
};

// The only one thread in the RUNNING state.
extern struct tcb *current_thread;
extern struct tcb *idle_thread;

struct tcb_queue {
    struct tcb *arr[THREAD_MAX];  // The circular array.
    int head;                     // The index of the head of the queue.
    int size;
};

// Zero initialized.
extern struct tcb_queue ready_queue, waiting_queue;

#define tcb_queue_push(queue, tcb)                             \
    do {                                                       \
        int next = ((queue).head + (queue).size) % THREAD_MAX; \
        (queue).arr[next] = tcb;                               \
        ++(queue).size;                                        \
    } while (0)

#define tcb_queue_head(queue) (queue).arr[(queue).head]

#define tcb_queue_pop(queue)                            \
    do {                                                \
        (queue).head = ((queue).head + 1) % THREAD_MAX; \
        --(queue).size;                                 \
    } while (0)

// The rwlock structure.
//
// When a thread acquires a type of lock, it should increment the corresponding count.
struct rwlock {
    int read_count;
    int write_count;
};

extern struct rwlock rwlock;

// The remaining spots in classes.
extern int q_p, q_s;

// The maximum running time for each thread.
extern int time_slice;

// The long jump buffer for the scheduler.
extern jmp_buf sched_buf;

// TODO::
// You should setup your own sleeping set as well as finish the marcos below
#define thread_create(func, t_id, t_args) ({ func(t_id, t_args); })

#define thread_setup(t_id, t_args)                                \
    ({                                                            \
        printf("thread %d: set up routine %s\n", t_id, __func__); \
        struct tcb *tcb = malloc(sizeof(struct tcb));             \
        tcb->id = t_id;                                           \
        tcb->args = t_args;                                       \
        tcb->waiting_for = 0;                                     \
        if (setjmp(tcb->env) == 0) {                              \
            if (t_id == 0) { /* Is idle thread. */                \
                idle_thread = tcb;                                \
            } else {                                              \
                tcb_queue_push(ready_queue, tcb);                 \
            }                                                     \
            return; /* Return to `spawn_thread()`. */             \
        }                                                         \
    })

#define thread_yield()                                      \
    ({                                                      \
        if (setjmp(current_thread->env) == 0) {             \
            sigset_t unblockset, oldset;                    \
            sigemptyset(&unblockset);                       \
            sigaddset(&unblockset, SIGTSTP);                \
            sigprocmask(SIG_UNBLOCK, &unblockset, &oldset); \
            sigprocmask(SIG_SETMASK, &oldset, NULL);        \
            sigemptyset(&unblockset);                       \
            sigaddset(&unblockset, SIGALRM);                \
            sigprocmask(SIG_UNBLOCK, &unblockset, &oldset); \
            sigprocmask(SIG_SETMASK, &oldset, NULL);        \
        }                                                   \
    })

#define read_lock()                                                      \
    ({                                                                   \
    })

#define write_lock()                                                     \
    ({                                                                   \
    })

#define read_unlock()                                                                 \
    ({                                                                                \
    })

#define write_unlock()                                                                \
    ({                                                                                \
    })

#define thread_sleep(sec)                                            \
    ({                                                               \
    })

#define thread_awake(t_id)                                                        \
    ({                                                                            \
    })

#define thread_exit()                                    \
    ({                                                   \
        printf("thread %d: exit\n", current_thread->id); \
        longjmp(sched_buf, SCHEDULER_JUMP_EXIT);         \
    })

#endif  // THREAD_TOOL_H
