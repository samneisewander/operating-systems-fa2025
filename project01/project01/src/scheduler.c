/* scheduler.c: PQSH Scheduler */

#include "pqsh/utils.h"
#include "pqsh/scheduler.h"
#include "pqsh/timestamp.h"

#include <errno.h>
#include <sys/wait.h>

/* Global Variables */

Scheduler PQSHScheduler = {
    .policy    = FIFO_POLICY,
    .cores     = 1,
    .timeout   = 250000,
    .event     = EVENT_INPUT,
};

/* Functions */

/**
 * Add new command to waiting queue.
 * @param   s       Pointer to Scheduler structure.
 * @param   command Command string for new Process.
 **/
void scheduler_add(Scheduler *s, const char *command) {
    /* TODO: Implement */

    if (!s) {
        fprintf(stderr, "scheduler_add: Scheduler is NULL.\n");
        return;
    }
    
    if (!command) {
        fprintf(stderr, "scheduler_add: Command is NULL.\n");
        return;
    }

    Process *p = process_create(command);

    if (!p) {
        fprintf(stderr, "scheduler_add: Failed to create process struct.\n");
        return;
    }

    /**
     * Assuming that the waiting queue exists and isn't full of garbage at this point.
     * I have no idea if that is something I should be assuming. Does statis memory get
     * initialized to zero at the start of program execution? What if the head pointer
     * in the waiting queue isn't NULL but points to trash? 
     */ 

    queue_push(&s->waiting, p);

}

/**
 * Display status of queues in Scheduler.
 * @param   s       Pointer to Scheduler structure.
 * @param   queue   Bitmask specifying which queues to display.
 **/
void scheduler_status(Scheduler *s, int queue) {
    if (!s) {
        fprintf(stderr, "scheduler_status: Scheduler is NULL\n");
        return;
    }

    // Google-san demands that you check for division by zero!!!! >:(
    double avg_turnaround = s->finished.size ? s->total_turnaround_time / s->finished.size : 0.0;
    double avg_response = s->finished.size ? s->total_response_time / s->finished.size : 0.0;

    printf("Running = %4lu, Waiting = %4lu, Finished = %4lu, Turnaround = %05.2lf, Response = %05.2lf\n", s->running.size, s->waiting.size, s->finished.size, avg_turnaround, avg_response);

    if (queue & WAITING && s->waiting.size > 0) {
        printf("Waiting Queue:\n");
        queue_dump(&s->waiting, stdout);
    } 
    
    if (queue & RUNNING && s->running.size > 0) {
        printf("Running Queue:\n");
        queue_dump(&s->running, stdout);
    } 
    
    if (queue & FINISHED && s->finished.size > 0) {
        printf("Finished Queue:\n");
        queue_dump(&s->finished, stdout);
    }
}

/**
 * Schedule next process using appropriate policy.
 * @param   s       Pointer to Scheduler structure.
 **/
void scheduler_next(Scheduler *s) {

    if (!s) {
        fprintf(stderr, "scheduler_next: Scheduler is NULL.\n");
        return;
    }

    switch(s->policy) {
        case FIFO_POLICY:
            scheduler_fifo(s);
            break;
        case RDRN_POLICY:
            scheduler_rdrn(s);
            break;
        default:
            fprintf(stderr, "scheduler_next: Invalid scheduling policy.\n");
            break;
    }

}

/**
 * Wait for any children and remove from queues and update metrics.
 * @param   s       Pointer to Scheduler structure.
 **/
void scheduler_wait(Scheduler *s) {

    pid_t pid;

    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
        Process *found = queue_remove(&s->running, pid);
        
        if (!found) {
            continue;
        }

        found->end_time = timestamp();
        queue_push(&s->finished, found);
        s->total_turnaround_time += found->end_time - found->arrival_time;
    }

}

/**
 * Handle SIGALRM by setting appropriate event.
 * @param   signum  Signal number (ignored).
 **/
void scheduler_handle_sigalrm(int signum) {
    /* Add EVENT_TIMER to PQSHScheduler. */
    // is something else supposed to happen here? Where is this timer property used?
    PQSHScheduler.event |= EVENT_TIMER;
}

/**
 * Handle SIGCHLD by setting appropriate event.
 * @param   signum  Signal number (ignored).
 **/
void scheduler_handle_sigchld(int signum) {
    /* Add EVENT_CHILD to PQSHScheduler. */
    PQSHScheduler.event |= EVENT_CHILD;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
