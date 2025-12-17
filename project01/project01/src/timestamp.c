/* timestamp.c: PQSH Timestamp */

#include "pqsh/timestamp.h"

#include <time.h>
#include <sys/time.h>
#include <stdbool.h>

#define BILLION 1000000000.0

/**
 * Return current timestamp as a double.
 *
 * Utilizes clock_gettime for precision (accounting for seconds and
 * nanoseconds) and falls back to time (only account for seconds) if that
 * fails.
 *
 * @return  Double representing the current time.
 **/
double timestamp() {

    struct timespec time_struct;
    double wall_time;

    if (clock_gettime(CLOCK_REALTIME, &time_struct) < 0) {
        wall_time = (double)time(NULL);
        if (wall_time < 0) {
            return -1;
        }
    } else {
        wall_time = time_struct.tv_sec + (time_struct.tv_nsec ) / BILLION;
    }

    return wall_time;

}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
