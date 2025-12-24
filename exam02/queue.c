/* queue.c: Queue (with two Condition Variables) */

#include "queue.h"

Queue* queue_create(char* sentinel, size_t capacity) {
    Queue* q = calloc(1, sizeof(Queue));
    if (q) {
        q->data = calloc(capacity, sizeof(char**));
        q->sentinel = sentinel;
        q->capacity = capacity;

        mutex_init(&q->lock, NULL);
        cond_init(&q->consumed, NULL);  // Initialize condition variable
        cond_init(&q->produced, NULL);  // Initialize condition variable
    }
    return q;
}

void queue_delete(Queue* q) {
    if (q) {
        free(q->data);
        free(q);
    }
}

void queue_push(Queue* q, char* value) {
    mutex_lock(&q->lock);

    while (q->size >= q->capacity) {
        cond_wait(&q->consumed, &q->lock);  // Yield and wait until woken up
    }

    // huh???
    /**
     * Okay, so after lecture i have learned that this is more deep C lore. Instead of using a linked list to implement the bounded buffer, we can use a regular int array and just keep track of the "writer" index and the "reader" index. Basically, every time we write, we advance writer. We loop around by modding the writer by the length of the bounded buffer. We do the same for the reader to keep track of what we've consumed so far.
     */
    q->data[q->writer] = value;
    q->writer = (q->writer + 1) % q->capacity;
    q->size++;

    cond_signal(&q->produced);  // Wake up consumer thread
    mutex_unlock(&q->lock);
}

char* queue_pop(Queue* q) {
    mutex_lock(&q->lock);

    while (q->size == 0) {
        cond_wait(&q->produced, &q->lock);  // Yield and wait until woken up
    }

    char* value = q->data[q->reader];
    if (value != q->sentinel) {
        q->reader = (q->reader + 1) % q->capacity;
        q->size--;
        cond_signal(&q->consumed);  // Wake up producer thread
    } else {
        cond_signal(&q->produced);  // Wake up producer thread
    }

    mutex_unlock(&q->lock);
    return value;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */