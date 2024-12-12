#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "routine.h"
#include "thread_tool.h"

// TODO::
// Prints out the signal you received.
// This function should not return. Instead, jumps to the scheduler.
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

static void print_queue(struct tcb_queue* queue) {
    fprintf(stderr, "head=%d, size=%d\n", queue->head, queue->size);
    for (int i = 0; i < queue->size; ++i) {
        int index = (queue->head + i) % THREAD_MAX;
        fprintf(stderr, "queue[%d]: tcb(.id=%d)\n", index, queue->arr[index]->id);
    }
}

// TODO::
// Perfectly setting up your scheduler.
void scheduler() {
    thread_create(idle, 0, NULL);

    // Handle jump from previously running thread.
    switch (setjmp(sched_buf)) {
        case SCHEDULER_JUMP_YIELD:
            tcb_queue_push(ready_queue, current_thread);
            break;
        case SCHEDULER_JUMP_EXIT:
            free(current_thread->args);
            free(current_thread);
            break;
    }
    current_thread = NULL;

    // Setup alarm to enforce time slicing.
    alarm(time_slice);

    // Clear pending signals.
    struct sigaction ignact, oldact;
    ignact.sa_handler = SIG_IGN;
    sigaction(SIGTSTP, &ignact, &oldact);
    sigaction(SIGTSTP, &oldact, NULL);
    sigaction(SIGALRM, &ignact, &oldact);
    sigaction(SIGALRM, &oldact, NULL);

    fprintf(stderr, "ready_queue:\n");
    print_queue(&ready_queue);

    // Every thread has exited.
    if (ready_queue.size == 0) {
        free(idle_thread);
        return;
    }
    // Run next thread.
    current_thread = tcb_queue_head(ready_queue);
    tcb_queue_pop(ready_queue);
    fprintf(stderr, "scheduler: running thread %d\n", current_thread->id);
    longjmp(current_thread->env, 1);
}
