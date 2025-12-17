/* test_client.c: Test SMQ Client (Functional) */

#include "smq/client.h"

#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>

/* Constants */

const char * TOPIC     = "testing";
const size_t NMESSAGES = 1<<4;

/* Globals */

sem_t Shutdown;

/* Threads */

void *incoming_thread(void *arg) {
    SMQ *smq = (SMQ *)arg;
    size_t messages = 0;

    while (smq_running(smq)) {
        char *message = smq_retrieve(smq);
        if (message) {
            assert(strstr(message, "Hello from"));
            free(message);
            messages++;
        }

        if (messages == NMESSAGES) {
            sem_post(&Shutdown);
        }
    }

    return NULL;
}

void *outgoing_thread(void *arg) {
    SMQ *smq = (SMQ *)arg;
    char body[BUFSIZ];

    for (size_t i = 0; i < NMESSAGES; i++) {
        sprintf(body, "%lu. Hello from %lu\n", i, time(NULL));
        smq_publish(smq, TOPIC, body);

        if (i % 4 == 0) {
            sleep(1);
        }
    }

    sem_wait(&Shutdown);
    smq_shutdown(smq);
    return NULL;
}

/* Main execution */

int main(int argc, char *argv[]) {
    /* Parse command-line arguments */
    char *name = getenv("USER");
    char *host = "localhost";
    char *port = "9620";

    if (argc > 1) { host = argv[1]; }
    if (argc > 2) { port = argv[2]; }
    if (!name)    { name = "test_client";  }

    /* Initialize semaphore */
    sem_init(&Shutdown, 0, 0);

    /* Create and start message queue */
    SMQ *smq = smq_create(name, host, port);
    assert(smq);

    smq_subscribe(smq, TOPIC);
    smq_unsubscribe(smq, TOPIC);
    smq_subscribe(smq, TOPIC);


    /* Run and wait for incoming and outgoing threads */
    Thread incoming;
    Thread outgoing;
    thread_create(&incoming, NULL, incoming_thread, smq);
    thread_create(&outgoing, NULL, outgoing_thread, smq);
    thread_join(incoming, NULL);
    thread_join(outgoing, NULL);

    smq_delete(smq);
    return 0;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
