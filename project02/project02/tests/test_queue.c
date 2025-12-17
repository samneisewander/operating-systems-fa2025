/* test_queue.c: Test SMQ Concurrent Queue of Requests (Functional) */

#include "smq/thread.h"
#include "smq/queue.h"
#include "smq/utils.h"

#include <assert.h>

/* Globals */

size_t NCONSUMERS = 2;
size_t NPRODUCERS = 4;
size_t NREQUESTS  = 1<<10;

/* Threads */

void *consumer(void *arg) {
    Queue *q = (Queue *)arg;
    size_t requests = 0;
    size_t methods  = 0;
    size_t urls     = 0;
    size_t bodies   = 0;

    while (requests < NREQUESTS) {
        Request *m = queue_pop(q, 1000);
        if (!m) {
            continue;
        }
        methods += atoi(m->method);
        urls    += atoi(m->url);
        bodies  += atoi(m->body);

        assert(streq(m->method, "1"));
        assert(streq(m->url   , "2"));
        assert(streq(m->body  , "3"));

        request_delete(m);
        requests++;
    }

    assert(methods == NREQUESTS);
    assert(urls    == NREQUESTS*2);
    assert(bodies  == NREQUESTS*3);
    return NULL;
}

void *producer(void *arg) {
    Queue *q = (Queue *)arg;
    char method[BUFSIZ];
    char url[BUFSIZ];
    char body[BUFSIZ];

    for (size_t m = 0; m < NREQUESTS; m++) {
        strcpy(method, "1");
        strcpy(url   , "2");
        strcpy(body  , "3");
        queue_push(q, request_create(method, url, body));
    }

    return NULL;
}

/* Main execution */

int main(int argc, char *argv[]) {
    if (argc > 1) NPRODUCERS = atoi(argv[1]);
    if (argc > 2) NCONSUMERS = atoi(argv[2]);
    if (argc > 3) NREQUESTS  = atoi(argv[3]);

    Thread producers[NPRODUCERS];
    Thread consumers[NCONSUMERS];
    Queue *q = queue_create();

    for (size_t c = 0; c < NCONSUMERS; c++) {
        thread_create(&consumers[c], NULL, consumer, q);
    }

    for (size_t p = 0; p < NPRODUCERS; p++) {
        thread_create(&producers[p], NULL, producer, q);
    }

    for (size_t p = 0; p < NPRODUCERS; p++) {
        thread_join(producers[p], NULL);
    }

    queue_shutdown(q);

    for (size_t c = 0; c < NCONSUMERS; c++) {
        thread_join(consumers[c], NULL);
    }

    queue_delete(q);
    return EXIT_SUCCESS;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
