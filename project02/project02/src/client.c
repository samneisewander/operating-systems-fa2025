/* client.c: Simple Request Queue Client */

#include "smq/client.h"

#define TIMEOUT 2000 /* Time (ms) to block before retrying pop. I put this here! - Sam. */

/* Internal Prototypes */

void *smq_pusher(void *);
void *smq_puller(void *);

/* External Functions */

/**
 * Create Simple Request Queue with specified name, host, and port.
 *
 * - Initialize values.
 * - Create internal queues.
 * - Create pusher and puller threads.
 *
 * @param   name        Name of client's queue.
 * @param   host        Address of server.
 * @param   port        Port of server.
 * @return  Newly allocated Simple Request Queue structure.
 **/
SMQ *smq_create(const char *name, const char *host, const char *port) {
    SMQ *smq = calloc(1, sizeof(SMQ));

    snprintf(smq->server_url, MAX_SERVER_URL_LEN, "%s:%s", host, port);
    snprintf(smq->name, MAX_CLIENT_NAME_LEN, "%s", name);

    smq->running = true;
    smq->timeout = TIMEOUT;
    smq->incoming = queue_create();
    smq->outgoing = queue_create();

    smq->puller = calloc(1, sizeof(Thread));
    smq->pusher = calloc(1, sizeof(Thread));
    smq->mutex = calloc(1, sizeof(Mutex));
    thread_create(smq->puller, NULL, smq_puller, (void *)smq);
    thread_create(smq->pusher, NULL, smq_pusher, (void *)smq);
    mutex_init(smq->mutex, NULL);

    return smq;
}

/**
 * Delete Simple Request Queue structure (and internal resources). May only be called
 * after smq_shutdown.
 * @param   smq     Simple Request Queue structure.
 **/
void smq_delete(SMQ *smq) {
    if (!smq) {
        return;
    }

    queue_delete(smq->incoming);
    queue_delete(smq->outgoing);
    free(smq->puller);
    free(smq->pusher);
    free(smq->mutex);
    free(smq);
}

/**
 * Publish one message to topic (by placing new Request in outgoing queue).
 * @param   smq     Simple Request Queue structure.
 * @param   topic   Topic to publish to.
 * @param   body    Request body to publish.
 **/
void smq_publish(SMQ *smq, const char *topic, const char *body) {
    char url[MAX_URL_LEN];

    // The topic will be truncated to fit into a buffer of size BUFSIZ
    snprintf(url, MAX_URL_LEN, "%s/%s/%s", smq->server_url, "topic", topic);
    Request *r = request_create("PUT", url, body);

    if (!r) {
        fprintf(stderr, "smq_publish: failed to create request.\n");
        request_delete(r);
        return;
    }

    queue_push(smq->outgoing, r);
}

/**
 * Retrieve one message (by taking a Request from incoming queue).
 *
 * Note: if the SMQ is no longer running, this will return NULL.
 *
 * @param   smq     Simple Request Queue structure.
 * @return  Newly allocated message body (must be freed).
 **/
char *smq_retrieve(SMQ *smq) {
    Request *r = queue_pop(smq->incoming, smq->timeout);

    if (!r) {
        return NULL;
    }

    char *body = strdup(r->body);
    request_delete(r);
    return body;
}

/**
 * Subscribe to specified topic.
 * @param   smq     Simple Request Queue structure.
 * @param   topic   Topic string to subscribe to.
 **/
void smq_subscribe(SMQ *smq, const char *topic) {
    char url[MAX_URL_LEN] = {0};

    // Assemble url of form [server url]/subscription/[client name]/[topic]
    // Example: http://10.0.0.1:1234/subscription/Sai/messages

    // The topic will be truncated to fit into a buffer of size BUFSIZ
    snprintf(url, MAX_URL_LEN, "%s/%s/%s/%s", smq->server_url, "subscription", smq->name, topic);
    Request *r = request_create("PUT", url, NULL);

    queue_push(smq->outgoing, r);
}

/**
 * Unubscribe to specified topic.
 * @param   smq     Simple Request Queue structure.
 * @param   topic   Topic string to unsubscribe from.
 **/
void smq_unsubscribe(SMQ *smq, const char *topic) {
    char url[MAX_URL_LEN];

    // The topic will be truncated to fit into a buffer of size BUFSIZ
    snprintf(url, MAX_URL_LEN, "%s/%s/%s/%s", smq->server_url, "subscription", smq->name, topic);

    Request *r = request_create("DELETE", url, NULL);
    queue_push(smq->outgoing, r);
}

/**
 * Shutdown the Simple Request Queue by:
 *
 * 1. Shutting down the internal queues.
 * 2. Setting the internal running attribute.
 * 3. Joining internal threads.
 *
 * @param   smq      Simple Request Queue structure.
 */
void smq_shutdown(SMQ *smq) {
    mutex_lock(smq->mutex);
    smq->running = false;
    mutex_unlock(smq->mutex);
    queue_shutdown(smq->outgoing);
    queue_shutdown(smq->incoming);
    thread_join(*smq->puller, NULL);
    thread_join(*smq->pusher, NULL);
}

/**
 * Returns whether or not the Simple Request Queue is running.
 * @param   smq     Simple Request Queue structure.
 **/
bool smq_running(SMQ *smq) {
    mutex_lock(smq->mutex);
    bool running = smq->running;
    mutex_unlock(smq->mutex);

    return running;
}

/* Internal Functions */

/**
 * Pusher thread takes messages from outgoing queue and sends them to server.
 **/
void *smq_pusher(void *arg) {
    SMQ *smq = (SMQ *)arg;
    Request *req = NULL;
    char *res = NULL;

    bool running = smq_running(smq);

    while (running) {
        req = queue_pop(smq->outgoing, smq->timeout);
        if (req) {
            res = request_perform(req, smq->timeout);
            if (!res) {
                fprintf(stderr, "pusher server: %s\n", res);
            } else {
                free(res);
            }
            request_delete(req);
        }

        running = smq_running(smq);
    }

    return NULL;
}

/**
 * Puller thread requests new messages from server and then puts them in
 * incoming queue.
 **/
void *smq_puller(void *arg) {
    SMQ *smq = (SMQ *)arg;

    char url[MAX_URL_LEN] = {0};
    char *res = NULL;
    snprintf(url, MAX_URL_LEN, "%s/%s/%s", smq->server_url, "queue", smq->name);

    bool running = smq_running(smq);

    while (running) {
        Request *req = request_create("GET", url, NULL);
        if (req) {
            res = request_perform(req, smq->timeout);
            if (res) {
                req->body = res;
                queue_push(smq->incoming, req);
            } else {
                request_delete(req);
            }
        } else {
            fprintf(stderr, "smq_puller: failed to create reqeust.\n");
        }

        running = smq_running(smq);
    }

    return NULL;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
