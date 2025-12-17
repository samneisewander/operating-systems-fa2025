/* scheduler.h: PQSH Scheduler */

#ifndef PQSH_SCHEDULER_H
#define PQSH_SCHEDULER_H

#include "pqsh/queue.h"

#include <stdio.h>

/* Constants */

typedef enum {
    FIFO_POLICY,        /* First in, first out */
    RDRN_POLICY,        /* Round robin */
} Policy;

typedef enum {
    EVENT_INPUT = 1<<0, /* Input event. What is this used for??? */
    EVENT_TIMER = 1<<1, /* Timer event */
    EVENT_CHILD = 1<<2, /* Child event */
} Event;

enum {
    RUNNING  = 1<<0,    /* Running queue */
    WAITING  = 1<<1,    /* Waiting queue */
    FINISHED = 1<<2,    /* Finished queue */
};

/* Structure */

typedef struct Scheduler Scheduler;

struct Scheduler {
    Policy  policy;     /* Scheduling policy */
    size_t  cores;      /* Number of CPU cores to utilize */
    time_t  timeout;    /* Time slice (microseconds) */
    Event   event;      /* Next event */

    Queue   running;    /* Queue of running processes */
    Queue   waiting;    /* Queue of waiting processes */
    Queue   finished;   /* Queue of finished processes */

    /* Total turnaround and response time */
    double  total_turnaround_time;
    double  total_response_time;
};

/* Commands */

void    scheduler_add(Scheduler *s, const char *command);
void    scheduler_status(Scheduler *s, int queue);

/* Functions */

void    scheduler_next(Scheduler *s);
void    scheduler_wait(Scheduler *s);

/* Signal Handlers */

void    scheduler_handle_sigalrm(int signum);
void    scheduler_handle_sigchld(int signum);

/* Policies */

void    scheduler_fifo(Scheduler *s);
void    scheduler_rdrn(Scheduler *s);

#endif

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
