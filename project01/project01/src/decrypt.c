#include "pqsh/decrypt.h"

int decrypt_egg() {
 
    unsigned char key[KEY_BYTES];
    unsigned char iv[IV_BYTES];
    
    /* In the future, I'd like to hash the symetric key and leave a clue to the egg in the source instead of the key itself.*/
    memcpy(key, "Prestidigitation", KEY_BYTES);
    memcpy(iv, "1234567890123456", IV_BYTES);

    char bindir[PATH_MAX];
    char *filename = "/../lib/cypher.txt";
    if (readlink("/proc/self/exe", bindir, PATH_MAX) < 0) {
        return -1;
    }

    // Get the directory name from the path
    char *dirpath = dirname(bindir);

    // Build the new path safely using snprintf
    char fullpath[PATH_MAX];
    snprintf(fullpath, PATH_MAX, "%s%s", dirpath, filename);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    const EVP_CIPHER *cipher = EVP_aes_128_cbc();
    
    if (!ctx || !cipher || !EVP_DecryptInit_ex(ctx, cipher, NULL, key, iv)) {
        fprintf(stderr, "Could not create cipher ctx.\n");
        return -1;
    }
    
    unsigned char in_buffer[BUFSIZ];
    unsigned char out_buffer[BUFSIZ];
    int inlen, outlen;
    
    // since operating on binary data need open in binary read mode
    FILE* infile = fopen(fullpath, "rb");
    if (!infile) {
        fprintf(stderr, "Could not open file. (errno %s)\n", strerror(errno));
        return -1;
    }

    while((inlen = fread(in_buffer, 1, BUFSIZ, infile)) > 0) {
        if (!EVP_DecryptUpdate(ctx, out_buffer, &outlen, in_buffer, inlen)) {
            fprintf(stderr, "Could not decrypt file.\n");
            return -1;
        }
        fwrite(out_buffer, 1, outlen, stdout);
    }

    if (!EVP_DecryptFinal(ctx, out_buffer, &outlen)) {
        fprintf(stderr, "Could not decrypt file.\n");
        return -1;
    }
    
    fwrite(out_buffer, 1, outlen, stdout);

    EVP_CIPHER_CTX_free(ctx);
    
    if (fclose(infile) == EOF) {
        fprintf(stderr, "decrypt_egg: Failed to close file descriptor.\n");
    };

    return 0;

}