/* Reading 05 */

#include <fcntl.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "queue.h"
#include "thread.h"

#define MAX_THREADS 2        /* Number of threads in thread pool */
#define QSIZE 4              /* Maximum size of Queue structure */
#define SENTINEL (char*)NULL /* Sentinel to mark end of Queue */

bool sha1sum_file(const char* path, char* cksum) {
    bool status = true;

    // get file descriptor for path and handle failure
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return false;
    }

    // create a blank digest context
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_sha1();  // i think this strategy is legacy, and EVP_MD_fetch should be used instead.

    if (!ctx) {
        status = false;
        goto cleanup;
    }

    // initialize SHA1 digest context and handle failure
    int success = EVP_DigestInit_ex(ctx, (const EVP_MD*)md, NULL);
    if (!success) {
        status = false;
        goto cleanup;
    }

    // allocate a buffer for reading file chunks and read the first chunk
    char chunk[BUFSIZ] = {0};
    ssize_t chunkSize = read(fd, chunk, BUFSIZ);

    // continue reading until EOF
    while (chunkSize) {
        // test cases do not catch read failure!!
        // handle read failure
        if (chunkSize < 0) {
            status = false;
            goto cleanup;
        }
        EVP_DigestUpdate(ctx, chunk, chunkSize);
        chunkSize = read(fd, chunk, BUFSIZ);
    }

    // compute the raw SHA1 digest
    unsigned char digest[EVP_MAX_MD_SIZE] = {0};
    EVP_DigestFinal_ex(ctx, digest, NULL);

    /* Convert raw SHA1 digest into hexadecimal cksum */
    for (int b = 0; b < SHA_DIGEST_LENGTH; b++) {
        snprintf(cksum + 2 * b, 3, "%02x", digest[b]);
    }

cleanup:
    if (fd) {
        close(fd);
    }

    if (ctx) {
        EVP_MD_CTX_free(ctx);
    }
    return status;
}

void* sha1_consumer(void* arg) {
    if (!arg) {
        fprintf(stderr, "sha1sum_threads: Expecting Queue *, not NULL.\n");
        return (void*)1;
    }

    Queue* q = (Queue*)arg;
    size_t failures = 0;
    char cksum[SHA_DIGEST_LENGTH * 2 + 1];
    char* path;

    // something something waait for the job queue to have stuff in it, then
    // yoink a job off the queue and sha1sum it, repeat this until nothing on the job
    // queue

    while ((path = queue_pop(q))) {
        if (!sha1sum_file(path, cksum)) {
            fprintf(stderr, "sha1sum_process: Hash failed for file %s\n", path);
            return (void*)1;
        }

        printf("%s  %s\n", cksum, path);
    }

    return (void*)failures;
}

int main(int argc, char* argv[]) {
    Queue* q = queue_create(SENTINEL, QSIZE);

    Thread threads[MAX_THREADS];
    size_t failures = 0;

    // Initialize thread pool
    for (int i = 0; i < MAX_THREADS; i++) {
        thread_create(&threads[i], NULL, sha1_consumer, (void*)q);
    }

    // Add jobs to the queue.
    for (int i = 1; i < argc; i++) {
        queue_push(q, argv[i]);
    }

    for (int i = 0; i < MAX_THREADS; i++) {
        queue_push(q, SENTINEL);
    }

    // Wait for threads to finish all jobs, count failures.
    size_t thread_failures = 0;
    for (int i = 0; i < MAX_THREADS; i++) {
        thread_join(threads[i], (void**)&thread_failures);
        failures += thread_failures;
    }

    // Delete queue after all threads have be joined
    queue_delete(q);

    return failures;
}