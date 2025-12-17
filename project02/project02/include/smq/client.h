/* client.h: Simple Message Queue client */

#ifndef SMQ_CLIENT_H
#define SMQ_CLIENT_H

#include <netdb.h>
#include <stdbool.h>
#include <time.h>

#include "smq/queue.h"

/**
 * Buffer size definitions. Goal here is to eliminate the
 * possibility of a buffer overflow by constrining URL
 * lengths always fit inside a buffer of size BUFSIZ.
 **/

#define MAX_URL_LEN BUFSIZ            // Maximum length of entire request URL.
#define MAX_SERVER_URL_LEN (1 << 8)   // Maximum length of protocol, host, and port portion of URL.
#define MAX_CLIENT_NAME_LEN (1 << 8)  // Maximum length of the client username.

/* Structures */

typedef struct {
    char name[MAX_CLIENT_NAME_LEN];       // Name of message queue
    char server_url[MAX_SERVER_URL_LEN];  // URL of server

    time_t timeout;  // Socket timeout (milliseconds)
    bool running;    // Whether or not SMQ is running (active)

    Queue *outgoing;  // Requests to be sent to server
    Queue *incoming;  // Requests received from server

    Thread *pusher;
    Thread *puller;

    Mutex *mutex;
} SMQ;

SMQ *smq_create(const char *name, const char *host, const char *port);
void smq_delete(SMQ *smq);

void smq_publish(SMQ *smq, const char *topic, const char *body);
char *smq_retrieve(SMQ *smq);

void smq_subscribe(SMQ *smq, const char *topic);
void smq_unsubscribe(SMQ *smq, const char *topic);

bool smq_running(SMQ *smq);
void smq_shutdown(SMQ *smq);

#endif

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
