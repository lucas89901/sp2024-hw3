#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "routine.h"
#include "thread_tool.h"

struct tcb* sleeping_set[THREAD_MAX];
int sleeping_count = 0;

__attribute__((noreturn)) void sighandler(int signum) {
    printf("caught ");
    switch (signum) {
        case SIGTSTP:
            printf("SIGTSTP");
            break;
        case SIGALRM:
            printf("SIGALRM");
            break;
    }
    printf("\n");
    longjmp(sched_buf, SCHEDULER_JUMP_YIELD);
}

#ifdef DEBUG
static void print_queue(struct tcb_queue* queue) {
    fprintf(stderr, "head=%d, size=%d\n", queue->head, queue->size);
    for (int i = 0; i < queue->size; ++i) {
        int index = (queue->head + i) % THREAD_MAX;
        fprintf(stderr, "queue[%d]: tcb(.id=%d)\n", index, queue->arr[index]->id);
    }
}
#endif

void scheduler() {
    thread_create(idle, 0, NULL);

    int thread_status = setjmp(sched_buf);
#ifdef DEBUG
    fprintf(stderr, "ready_queue:\n");
    print_queue(&ready_queue);
    fprintf(stderr, "waiting_queue:\n");
    print_queue(&waiting_queue);
#endif

    // Setup alarm to enforce time slicing.
    alarm(time_slice);

    // Clear pending signals.
    struct sigaction ignact, oldact;
    ignact.sa_handler = SIG_IGN;
    sigaction(SIGTSTP, &ignact, &oldact);
    sigaction(SIGTSTP, &oldact, NULL);
    sigaction(SIGALRM, &ignact, &oldact);
    sigaction(SIGALRM, &oldact, NULL);

    // Handle sleeping threads.
    if (thread_status == SCHEDULER_JUMP_YIELD) {
        for (int i = 0; i < THREAD_MAX; ++i) {
            if (!sleeping_set[i]) {
                continue;
            }
            sleeping_set[i]->sleeping_time -= time_slice;
            if (sleeping_set[i]->sleeping_time <= 0) {
                tcb_queue_push(ready_queue, sleeping_set[i]);
                sleeping_set_remove(sleeping_set[i]);
            }
        }
    }

    // Handle waiting threads.
    while (waiting_queue.size > 0) {
        if (tcb_queue_head(waiting_queue)->waiting_for == WAITING_READ) {
            if (rwlock.write_count == 0) {
                tcb_queue_push(ready_queue, tcb_queue_head(waiting_queue));
                tcb_queue_pop(waiting_queue);
            } else {
                break;
            }
        } else {  // WAITING_WRITE
            if (rwlock.read_count == 0 && rwlock.write_count == 0) {
                tcb_queue_push(ready_queue, tcb_queue_head(waiting_queue));
                tcb_queue_pop(waiting_queue);
            } else {
                break;
            }
        }
    }

    // Handle jump from previously running thread.
    switch (thread_status) {
        case SCHEDULER_JUMP_YIELD:
            if (current_thread != idle_thread) {
                tcb_queue_push(ready_queue, current_thread);
            }
            break;
        case SCHEDULER_JUMP_EXIT:
            free(current_thread->args);
            free(current_thread);
            break;
        case SCHEDULER_JUMP_SLEEP:
            // The thread is already in the sleeping set.
            break;
        case SCHEDULER_JUMP_WAIT:
            tcb_queue_push(waiting_queue, current_thread);
            break;
    }
    current_thread = NULL;

    if (ready_queue.size == 0) {
        if (sleeping_count > 0) {
            current_thread = idle_thread;
            longjmp(idle_thread->env, 1);
        }
        // Every thread has exited.
        free(idle_thread);
        return;
    }
    // Run next thread.
    current_thread = tcb_queue_head(ready_queue);
    tcb_queue_pop(ready_queue);
#ifdef DEBUG
    fprintf(stderr, "scheduler: running thread %d\n", current_thread->id);
#endif
    longjmp(current_thread->env, 1);
}
