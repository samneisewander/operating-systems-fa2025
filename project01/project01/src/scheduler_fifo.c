/* scheduler_fifo.c: PQSH FIFO Scheduler */

#include "pqsh/utils.h"
#include "pqsh/scheduler.h"
#include "pqsh/timestamp.h"

#include <assert.h>
#include <signal.h>

/**
 * Schedule next process using fifo policy:
 *
 *  As long as we have less running processes than our number of CPUS and
 *  there are waiting processes, then we should move a process from the
 *  waiting queue to the running queue.
 *
 * @param   s       Scheduler structure
 **/
void scheduler_fifo(Scheduler *s) {

    Process *p = NULL;

    while(s->running.size < s->cores && s->waiting.size > 0) {
        p = queue_pop(&s->waiting);
        
        if (!p) {
            fprintf(stderr, "scheduler_fifo: Failed to pop from waiting queue.\n");
            return;
        }

        if (process_start(p)) {
            queue_push(&s->running, p);

            // Assuming start_time and arrival_time are set
            s->total_response_time += p->start_time - p->arrival_time;
        } else {
            fprintf(stderr, "scheduler_fifo: Failed to start process.\n");
            goto error;
        }
    }

    return;

    error:
        // KILL IT WITH FIRE.
        if (p) {
            p->end_time = timestamp();
            p->next = NULL;
            if (p->pid) {
                kill(p->pid, 9);
            }
            queue_push(&s->finished, p);
        }
        return;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
