/* queue.c: Concurrent Queue of Requests */

#include "smq/queue.h"

#include <time.h>
#define MILLION 1000000

#include "smq/utils.h"

/**
 * Create queue structure.
 * @return  Newly allocated queue structure, or NULL on failure.
 **/
Queue *queue_create() {
    Queue *q = calloc(1, sizeof(Queue));

    if (!q) {
        fprintf(stderr, "calloc failure: %s\n", strerror(errno));
        return NULL;
    }

    q->running = true;
    mutex_init(&q->mutex, NULL);
    cond_init(&q->produced, NULL);

    return q;
}

/**
 * Delete queue structure. Must be called after all threads have been rejoined.
 * @param   q       Queue structure.
 **/
void queue_delete(Queue *q) {
    if (!q) {
        fprintf(stderr, "Attend to delete NULL queue.\n");
        return;
    }

    Request *curr = q->head;
    Request *next;

    while (curr) {
        next = curr->next;
        request_delete(curr);
        curr = next;
    }

    free(q);
}

/**
 * Shutdown queue by setting running to false.
 * @param   q       Queue structure.
 **/
void queue_shutdown(Queue *q) {
    if (!q) {
        fprintf(stderr, "Attempt to shutdown NULL queue.\n");
        return;
    }

    mutex_lock(&q->mutex);
    q->running = false;
    mutex_unlock(&q->mutex);
}

/**
 * Push message to the back of queue.
 * @param   q       Queue structure.
 * @param   r       Request structure.
 **/
void queue_push(Queue *q, Request *r) {
    if (!q) {
        fprintf(stderr, "Attempt to push to NULL queue.\n");
        return;
    }

    if (!r) {
        fprintf(stderr, "Attempt to push NULL to queue.\n");
        return;
    }

    mutex_lock(&q->mutex);
    if (!q->running) {
        mutex_unlock(&q->mutex);
        return;
    }

    if (q->size > 0) {
        q->tail->next = r;
        q->tail = r;
    } else {
        q->tail = r;
        q->head = r;
    }
    q->size++;
    mutex_unlock(&q->mutex);
    cond_signal(&q->produced);
}

/**
 * Pop message from the front of queue (block until there is something to return).
 * @param   q       Queue structure.
 * @param   timeout How long to wait before re-checking condition (ms).
 * @return  Request structure, or NULL on failure or if queue is not running.
 **/
Request *queue_pop(Queue *q, time_t timeout) {
    if (!q) {
        fprintf(stderr, "Attempt to pop from NULL queue.\n");
        return NULL;
    }

    // Setup timeout struct
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    t.tv_sec += timeout / 1000;
    t.tv_nsec += (timeout % 1000) * MILLION;

    // Lock and enter critical section.
    mutex_lock(&q->mutex);
    while (q->size <= 0) {
        cond_timedwait(&q->produced, &q->mutex, &t);
        // Check after every timeout or signal if the user has called queue_shutdown.
        if (!q->running) {
            mutex_unlock(&q->mutex);
            return NULL;
        }
    }

    // Perform the pop.
    Request *r = q->head;
    q->head = q->head->next;
    r->next = NULL;
    q->size--;
    mutex_unlock(&q->mutex);

    return r;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
