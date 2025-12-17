/* queue.c: PQSH Queue */

#include "pqsh/utils.h"
#include "pqsh/queue.h"

#include <assert.h>

/**
 * Push process to back of queue.
 * @param q     Pointer to Queue structure.
 **/
void queue_push(Queue *q, Process *p) {

    if (!q) {
        return;
    }

    if (!q->head || !q->tail){
        q->head = p;
        q->tail = p;
    } else {
        q->tail->next = p;
        q->tail = p;
    }

    p->next = NULL;
    q->size++;

}

/**
 * Pop process from front of queue.
 * @param q     Pointer to Queue structure.
 * @return  Process from front of queue.
 **/
Process * queue_pop(Queue *q) {
    
    if (!q) {
        fprintf(stderr, "queue_pop: Queue is NULL.\n");
        return NULL;
    }
    
    Process *p = q->head;
    if (!p) {
        fprintf(stderr, "queue_pop: Queue head is NULL.\n");
        return NULL;
    }
    
    q->head = p->next;
    q->size--;

    // Clean up dangling tail pointer if this is the last node in the list
    if (!q->head) {
        q->tail = NULL;
    }

    p->next = NULL;

    return p;
}

/**
 * Remove and return process with specified pid.
 * @param q     Pointer to Queue structure.
 * @param pid   Pid of process to return.
 * @return  Process from Queue with specified pid.
 **/
Process * queue_remove(Queue *q, pid_t pid) {

    if (!q || !q->head) {
        fprintf(stderr, "queue_remove: Queue is NULL.\n");
        return NULL;
    }
    
    if (q->head->pid == pid) {
        return queue_pop(q);
    }

    Process *curr = q->head;
    Process *prev = NULL;

    while (curr->next) {
        if (curr->pid == pid) {
            prev->next = curr->next;
            curr->next = NULL;
            q->size--;

            return curr;
        }

        prev = curr;
        curr = curr->next;
    }

    if (curr->pid == pid) {
        prev->next = NULL;
        curr->next = NULL;
        q->size--;
        return curr;
    }

    return NULL;
}

/**
 * Dump the contents of the Queue to the specified stream.
 * @param q     Queue structure.
 * @param fs    Output file stream.
 **/
void queue_dump(Queue *q, FILE *fs) {

    fprintf(fs, "%9s %-30s %-13s %-13s %-13s\n",
                "PID", "COMMAND", "ARRIVAL", "START", "END");

    for (Process *curr = q->head; curr; curr = curr->next) {
        fprintf(fs, "%9d %-30s %-13.2lf %-13.2lf %-13.2lf\n", curr->pid, curr->command, curr->arrival_time, curr->start_time, curr->end_time);
    }

}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
