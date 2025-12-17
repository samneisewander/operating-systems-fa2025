/* Reading 02 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

const char* egg = "  *    .       *      *    .         .         .     *    . \n      *    .       *      when its 2am in Fitz  .         .   \n     *     .       .       *  i like to imagine        .   \n  *    .       *      *    .     that one day i'll live .   \n          *     .       .       *    in a place like this  .   \n  *    .       *      *    .         .         .     *    . \n          *     .       .       *          .      .      . \n    .                  .-.    .  _   *     _   . \n           *          /   \\     ((       _/ \\       *    . \n         _    .   .--'\\/\\_ \\     `      /    \\  *    ___ \n     *  / \\_    _/ ^      \\/\'__        /\\/\\  /\\  __/   \\ * \n       /    \\  /    .'   _/  /  \\  *' /    \\/  \\/ .`'\\_/\\   . \n  .   /\\/\\  /\\/ :' __  ^/  ^/    `--./.'  ^  `-.\\ _    _:\\ _ \n     /    \\/  \\  _/  \\-' __/.' ^ _   \\_   .'\\   _/ \\ .  __/ \\ \n   /\\  .-   `. \\/     \\ / -.   _/ \\ -. `_/   \\ /    `._/  ^  \\ \n  /  `-.__ ^   / .-'.--'    . /    `--./ .-'  `-.  `-. `.  -  `. \n@/        `.  / /      `-.   /  .-'   / .   .'   \\    \\  \\  .-  \\ \n@&8jgs@@%% @)&@&(88&@.-_=_-=_-=_-=_-=_.8@%% &@&&8(8%%@%%8)(8@%%8 8%%@)\\%% \n@88:::&(&8&&8:::::%%&`.~-_~~-~~_~-~_~-~~=.'@(&%%::::%%@8&8)::&#@8:::: \n`::::::8%%@@%%:::::@%%&8:`.=~~-.~~-.~~=..~'8::::::::&@8:::::&8:::::' \n `::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::.' \n";

/**
 * NOTES: 
 *  - EVP stands for the digital EnVeloPe.
 *  - According to Gemini: A digest, also known as a hash, is a fixed-size string of bytes generated from a block of data
 *    so basically a digest is the same thing as a hash. Used to verify data integrity.
 *  - According to Gemini: A context is a structure that stores the state of a cryptographic operation, including the
 *    digest algorithm being used (like SHA1 or SHA256) and the intermediate state as the hash is computed in chunks.
 * 
 *  - EVP_MD_CTX_new: Create a new, empty digest context for writing a hash to. Think of this like a fresh blackboard.
 *  - EVP_DigestInit_ex: initializes a digest context with a specific algorithm.
 *  - EVP_DigestUpdate: processes a chunk of data and updates the digest context so you don't have to cram the entire
 *    file in memory.
 *  - EVP_DigestFinal_ex: This is the final step where you take a snapshot sof the completed notes on the blackboard,  
 *    effectively producing the final digest. After this, the blackboard is cleaned and the context is no longer usable 
 *    for further hashing.
 *  - EVP_MD_CTX_free: Free memory allocated for the digest context.
 * 
 *  - "MD" stands for "message digest"
 * 
 *  >>> MD FETCH FUNCTION >>>
 *  The EVP_MD_fetch function takes three main arguments:
  
    OSSL_LIB_CTX *ctx: This is a library context, which manages loaded providers and other configuration. You can pass NULL to use the default global context.

    const char *algorithm: This is a string specifying the name of the digest algorithm you want to use, such as "SHA256", "SHA3-512", or "MD5".

    const char *properties: This is a property query string that allows you to specify a preferred implementation. For example, you might use "provider=fips" to request a FIPS-compliant implementation or "provider=default" to use the default provider. You can pass NULL to use the default properties.

    >>> ENGINE pointer >>>
    In the EVP_DigestInit_ex function prototype, the ENGINE *impl argument specifies an OpenSSL Engine to use for the cryptographic operation.

    Optional: This argument is almost always set to NULL in modern applications.
    Legacy: The use of Engines is considered a legacy API in OpenSSL 3.0 and later. The modern, more flexible approach is to use Providers (like EVP_MD_fetch)

    >> TEST SCRIPTS >>
    - The test scripts (and valgrind) do not catch hanging pointers to message digests
      in the case where a syscall to open or read fails. 
 * 
 */
void cleanup(int fd, EVP_MD_CTX *ctx) {
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
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    const EVP_MD *md = EVP_sha1(); // i think this strategy is legacy, and EVP_MD_fetch should be used instead.
    
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
        snprintf(cksum + 2*b, 3, "%02x", digest[b]);
    }
    
    // cleanup
    cleanup(fd, ctx);

    return true;
}

int main(int argc, char *argv[]) {

    int failures = 0;

    // Gaurd improper usage.
    if (argc < 2) {
        fprintf(stderr, "Usage: program [file1 file2 ...]\n");
        return failures; // why is improper usage considered successful? this should be EXIT_FAILURE
    }

    /**
     * Buffer for message digest. The SHA_DIGEST_LENGTH is 20 bytes,
     * but since each byte is being converted to hex representation by sha1sum_file
     * you need to allocate 2 bytes for each raw digest byte, plus an additional 
     * byte for the null terminator.
     */
    char md[SHA_DIGEST_LENGTH * 2 + 1] = {0};

    // compute sha1sum for each argument
    for (int i = 1; i < argc; i++){
        if (strcmp(argv[i], "egg") == 0) {
            puts(egg);
        }
        else if (sha1sum_file(argv[i], md)) {
            printf("%s  %s\n", md, argv[i]);
        }
        else {
            failures++;
        }
    }

    return failures;
}


