/* Reading 04 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>

bool sha1sum_file(const char* path, char* cksum) {

    bool status = true;

    // get file descriptor for path and handle failure
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return false;
    }
    
    // create a blank digest context
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    const EVP_MD *md = EVP_sha1(); // i think this strategy is legacy, and EVP_MD_fetch should be used instead.
    
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
        snprintf(cksum + 2*b, 3, "%02x", digest[b]);
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

void *sha1sum_threads(void *arg) {
    
    if (!arg) {
        fprintf(stderr, "sha1sum_threads: Argument must not be NULL.\n");
        return (void *) 1;
    }
    
    char *path = (char *)arg;

    /**
     * Buffer for message digest. The SHA_DIGEST_LENGTH is 20 bytes,
     * but since each byte is being converted to hex representation by sha1sum_file
     * you need to allocate 2 bytes for each raw digest byte, plus an additional 
     * byte for the null terminator.
     */
    char cksum[SHA_DIGEST_LENGTH * 2 + 1];

    if (!sha1sum_file(path, cksum)) {
        fprintf(stderr, "sha1sum_process: Hash failed for file %s\n", path);
        return (void *) 1;
    }
    
    printf("%s  %s\n", cksum, path);
    
    return (void *) 0;
}

int main(int argc, char *argv[]) {
    
    pthread_t threads[argc - 1];
    int failures = 0;
    
    for (int i = 1; i < argc; i++) {
        pthread_create(&threads[i - 1], NULL, sha1sum_threads, argv[i]);
    }

    /**
     * THIS is some evil voodoo witchcraft forbidden C knowledge. Learning
     * about this has unwinded my sanity and crushed my faith in humanity.
     * 
     * If you declare the status as a regular int, which is 4 bytes, and then
     * pass a pointer to it into pthread_join casted as a void**, then 
     * pthread_join will write the return value (which is 8 bytes) over that region
     * of memory. By SHEER COINCIDENCE, that extra 4 bytes JUST HAPPENED to be the 
     * FAILURES int!!!!!! Meaning it resets failrures to zero and breaks the program.
     * 
     * The fix is to declare status as a long long int (8 bytes) so that the stack
     * size lines up with the pthread_join return value size.
     * 
     * I AM SO MAD.
     * 
     * WHY CANT A THREAD JUST RETURN A NORMAL TYPE????
     * 
     * Henry says it's because C doesn't support function overloading.
     * 
     * WELL GET TF ON IT C DEVELOPERS!!!! I'M MAD!!!!
     */
    long long int status;

    for (int i = 0; i < argc - 1; i++) {
        pthread_join(threads[i], (void **) &status);
        failures += status;
    }
    
    return failures;
}