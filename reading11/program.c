/* Reading 11 */

#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void cleanup(int fd, EVP_MD_CTX* ctx) {
    close(fd);
    EVP_MD_CTX_free(ctx);
}

bool sha1sum_file(const char* path, char* cksum) {
    // get file descriptor for path and handle failure
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return false;
    }

    // create a blank digest context
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_sha1();  // i think this strategy is legacy, and EVP_MD_fetch should be used instead.

    // initialize SHA1 digest context and handle failure
    int success = EVP_DigestInit_ex(ctx, (const EVP_MD*)md, NULL);
    if (!success) {
        cleanup(fd, ctx);
        return false;
    }

    // allocate a buffer for reading file chunks and read the first chunk
    char chunk[BUFSIZ] = {0};
    ssize_t chunkSize = read(fd, chunk, BUFSIZ);

    // continue reading until EOF
    while (chunkSize) {
        // test cases do not catch read failure!!
        // handle read failure
        if (chunkSize < 0) {
            cleanup(fd, ctx);
            return false;
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

    // cleanup
    cleanup(fd, ctx);

    return true;
}

int cksum_file(char* path) {
    char md[SHA_DIGEST_LENGTH * 2 + 1] = {0};
    if (sha1sum_file(path, md)) {
        printf("%s  %s\n", md, path);
        return 0;
    }
    return 1;
}

int cksum_directory(char* root) {
    DIR* dp = opendir(root);
    if (dp == NULL) {
        return 1;
    }

    char abspath[PATH_MAX];
    int status = 0;
    struct dirent* d;
    while ((d = readdir(dp)) != NULL) {
        if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0) {
            continue;
        }

        snprintf(abspath, PATH_MAX, "%s/%s", root, d->d_name);
        if (d->d_type == DT_DIR) {
            status += cksum_directory(abspath);
        } else if (d->d_type == DT_REG) {
            status += cksum_file(abspath);
        }
    }

    closedir(dp);
    return status;
}

int main(int argc, char* argv[]) {
    int failures = 0;

    // Gaurd improper usage.
    if (argc < 2) {
        fprintf(stderr, "Usage: program [file1 file2 ...]\n");
        return failures;  // why is improper usage considered successful? this should be EXIT_FAILURE
    }

    // recursively compute sha1sum for each argument
    for (int i = 1; i < argc; i++) {
        struct stat s;
        if (stat(argv[i], &s) < 0) {
            failures++;
            continue;
        }

        if (S_ISDIR(s.st_mode)) {
            failures += cksum_directory(argv[i]);
        } else if (S_ISREG(s.st_mode)) {
            failures += cksum_file(argv[i]);
        } else {
            failures++;
        }
    }

    return failures;
}
