#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread_tool.h"

void idle(int id, int *args) {
    thread_setup(id, args);
    while (1) {
        printf("thread %d: idle\n", current_thread->id);
        sleep(1);
        thread_yield();
    }
}

void fibonacci(int id, int *args) {
    thread_setup(id, args);

    current_thread->n = current_thread->args[0];
    for (current_thread->i = 1;; current_thread->i++) {
        if (current_thread->i <= 2) {
            current_thread->f_cur = 1;
            current_thread->f_prev = 1;
        } else {
            int f_next = current_thread->f_cur + current_thread->f_prev;
            current_thread->f_prev = current_thread->f_cur;
            current_thread->f_cur = f_next;
        }
        printf("thread %d: F_%d = %d\n", current_thread->id, current_thread->i,
               current_thread->f_cur);

        sleep(1);

        if (current_thread->i == current_thread->n) {
            thread_exit();
        } else {
            thread_yield();
        }
    }
}

void pm(int id, int *args) {
    thread_setup(id, args);

    current_thread->n = current_thread->args[0];
    for (current_thread->i = 1;; current_thread->i++) {
        if (current_thread->i == 1) {
            current_thread->f_cur = 1;
        } else {
            int sign = 1;
            // Minus if n is even.
            if ((current_thread->i & 1) == 0) {
                sign = -1;
            }
            current_thread->f_cur += sign * current_thread->i;
        }
        printf("thread %d: pm(%d) = %d\n", current_thread->id, current_thread->i, current_thread->f_cur);
        sleep(1);
        if (current_thread->i == current_thread->n) {
            thread_exit();
        } else {
            thread_yield();
        }
    }
}

void enroll(int id, int *args) {
    thread_setup(id, args);

    // Label for awake.
    if (setjmp(current_thread->env) == 0) {
        thread_sleep(current_thread->args[2]);  // s
    }
    thread_awake(current_thread->args[3]);  // b

    read_lock();
    printf("thread %d: acquire read lock\n", current_thread->id);
    sleep(1);
    thread_yield();

    current_thread->p_p = current_thread->args[0] /* d_p */ * q_p;
    current_thread->p_s = current_thread->args[1] /* d_s */ * q_s;
    read_unlock();
    printf("thread %d: release read lock, p_p = %d, p_s = %d\n", current_thread->id, current_thread->p_p,
           current_thread->p_s);
    sleep(1);
    thread_yield();

    write_lock();
    int sw = 0;  // Chosen class is sw.
    if (current_thread->p_p == current_thread->p_s) {
        sw = current_thread->args[0] < current_thread->args[1];
    } else {
        sw = current_thread->p_p < current_thread->p_s;
    }
    // Either class is full.
    if (q_s == 0) {
        sw = 0;
    }
    if (q_p == 0) {
        sw = 1;
    }
    printf("thread %d: acquire write lock, enroll in ", current_thread->id);
    if (sw) {
        --q_s;
        printf("sw");
    } else {
        --q_p;
        printf("pj");
    }
    printf("_class\n");
    sleep(1);
    thread_yield();

    write_unlock();
    printf("thread %d: release write lock\n", current_thread->id);
    sleep(1);
    thread_exit();
}
