/* Reading 02 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

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

void sha1sum_process(const char *path) {
    
    if (!path) {
        exit(EXIT_FAILURE);
    }

    /**
     * Buffer for message digest. The SHA_DIGEST_LENGTH is 20 bytes,
     * but since each byte is being converted to hex representation by sha1sum_file
     * you need to allocate 2 bytes for each raw digest byte, plus an additional 
     * byte for the null terminator.
     */
    char cksum[SHA_DIGEST_LENGTH * 2 + 1];

    if (!sha1sum_file(path, cksum)) {
        fprintf(stderr, "sha1sum_process: Hash failed for file %s\n", path);
        exit(EXIT_FAILURE);
    }

    printf("%s  %s\n", cksum, path);
    exit(EXIT_SUCCESS);

}

int main(int argc, char *argv[]) {

    int failures = 0;
    int status;

    // Gaurd improper usage.
    if (argc < 2) {
        fprintf(stderr, "Usage: program [file1 file2 ...]\n");
        return failures; // why is improper usage considered successful? this should be EXIT_FAILURE
    }

    // compute sha1sum for each argument
    for (int i = 1; i < argc; i++){

        pid_t pid = fork();

        if (pid == 0) {
            // child
            sha1sum_process(argv[i]);
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            // failure
            return EXIT_FAILURE;
        }

    }

    while (wait(&status) > 0) {
        failures += WEXITSTATUS(status);
    }
    
    return failures;
}


