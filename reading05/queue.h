/* queue.h: Queue */

#ifndef QUEUE_H
#define QUEUE_H

#include "thread.h"

/* Structures */

typedef struct Queue Queue;
struct Queue {
    char **data;
    char *sentinel;
    size_t capacity;

    size_t size;
    size_t reader;
    size_t writer;

    Mutex lock;  // Queue 1
    Cond cond;   // Queue 2

    Cond consumed;  // Queue 3
    Cond produced;  // Queue 3
};

/* Functions */

Queue *queue_create(char *sentinel, size_t capacity);
void queue_delete(Queue *q);

void queue_push(Queue *q, char *value);
char *queue_pop(Queue *q);

#endif

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */