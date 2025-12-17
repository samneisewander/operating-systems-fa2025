/* scheduler_rdrn.c: PQSH Round Robin Scheduler */

#include "pqsh/utils.h"
#include "pqsh/scheduler.h"
#include "pqsh/timestamp.h"

#include <assert.h>
#include <signal.h>

/**
 * Schedule next process using round robin policy:
 *
 *  1. If there no waiting processes, then do nothing.
 *
 *  2. Move one process from front of running queue and place in back of
 *  waiting queue.
 *
 *  3. Move one process from front of waiting queue and place in back of
 *  running queue.
 *
 * @param   s       Scheduler structure
 **/
void scheduler_rdrn(Scheduler *s) {
    
    /* Rotate processes until there are none waiting or we've swapped all cores */
    Process *p = NULL;

    /* Move all running processes to waiting queue */
    while (s->running.size > 0) {
        p = queue_pop(&s->running);
    
        if (p && process_pause(p)) {
            queue_push(&s->waiting, p);
        } else {
            fprintf(stderr, "scheduler_rdrn: Failed to pop from running queue.\n");
            goto error;
        }
    }

    /* Fill up all cores with waiting processes */
    while(s->waiting.size > 0 && s->running.size < s->cores) {
        p = queue_pop(&s->waiting);
        
        if (!p) {
            fprintf(stderr, "scheduler_rdrn: Failed to pop from waiting queue.\n");
            goto error;
        }

        if (!p->pid) {
            if(!process_start(p)) {
                fprintf(stderr, "scheduler_rdrn: Failed to start process.\n");
                goto error;
            }

            // Assuming start_time and arrival_time are set
            s->total_response_time += p->start_time - p->arrival_time;

        } else {
            if(!process_resume(p)) {
                fprintf(stderr, "scheduler_rdrn: Failed to resume process.\n");
                goto error;
            }
        }

        queue_push(&s->running, p);
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
