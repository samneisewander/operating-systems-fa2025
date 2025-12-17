/* Reading 06 */

#include <errno.h>
#include <fcntl.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_THREADS 2 /* Maximum number of active threads */

sem_t Lock;          /* Semaphore for synchronizing access to Failures */
sem_t Running;       /* Semaphore for throttling number of active threads to MAX_THREADS */
size_t Failures = 0; /* Records number of failed sha1sum computations */

bool sha1sum_file(const char *path, char *cksum) {
    bool status = true;

    // get file descriptor for path and handle failure
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return false;
    }

    // create a blank digest context
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    const EVP_MD *md = EVP_sha1();  // i think this strategy is legacy, and EVP_MD_fetch should be used instead.

    if (!ctx) {
        status = false;
        goto cleanup;
    }

    // initialize SHA1 digest context and handle failure
    int success = EVP_DigestInit_ex(ctx, (const EVP_MD *)md, NULL);
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
    if (fd > 0) {
        close(fd);
    }

    if (ctx) {
        EVP_MD_CTX_free(ctx);
    }
    return status;
}

void *sha1sum_threads(void *arg) {
    sem_wait(&Running);

    if (!arg) {
        fprintf(stderr, "sha1sum_threads: Argument must not be NULL.\n");
        return (void *)1;
    }

    char *path = (char *)arg;

    char cksum[SHA_DIGEST_LENGTH * 2 + 1];

    if (!sha1sum_file(path, cksum)) {
        sem_wait(&Lock);
        Failures++;
        sem_post(&Lock);
    } else {
        printf("%s  %s\n", cksum, path);
    }

    sem_post(&Running);
    return 0;
}

int main(int argc, char *argv[]) {
    if (sem_init(&Lock, 0, 1) < 0) {
        fprintf(stderr, "Could not initialize semaphore: %s\n", strerror(errno));
        return 1;
    }
    if (sem_init(&Running, 0, MAX_THREADS) < 0) {
        fprintf(stderr, "Could not initialize semaphore: %s\n", strerror(errno));
        return 1;
    }

    pthread_t threads[argc - 1];

    for (int i = 1; i < argc; i++) {
        if (pthread_create(&threads[i - 1], NULL, sha1sum_threads, argv[i])) {
            fprintf(stderr, "Failed to create thread. Retrying...\n");
            i--;
        }
    }

    for (int i = 0; i < argc - 1; i++) {
        pthread_join(threads[i], NULL);
    }

    return Failures;
}