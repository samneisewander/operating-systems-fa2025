#include <openssl/err.h>
#include <openssl/evp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool validateKeyHash(unsigned char *hash) {
    const unsigned char keyhash_data[] = {0xB4, 0x74, 0xA9, 0x9A, 0x27, 0x05, 0xE2, 0x3C, 0xF9, 0x05, 0xA4, 0x84, 0xEC, 0x6D, 0x14, 0xEF, 0x58, 0xB5, 0x6B, 0xBE, 0x62, 0xE9, 0x29, 0x27, 0x83, 0x46, 0x6E, 0xC3, 0x63, 0xB5, 0x07, 0x2D};

    for (int i = 0; i < 32; i++) {
        if (hash[i] != keyhash_data[i]) {
            return false;
        }
    }

    return true;
}

unsigned char *hashKey(const char *plaintextKey, unsigned int *out_len) {
    if (!plaintextKey || !out_len) return NULL;

    EVP_MD_CTX *mdctx = NULL;
    unsigned char *digest = NULL;
    unsigned int digest_len = 0;

    mdctx = EVP_MD_CTX_new();
    if (!mdctx) goto fail;

    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) != 1) goto fail;
    if (EVP_DigestUpdate(mdctx, plaintextKey, strlen(plaintextKey)) != 1) goto fail;

    digest = malloc(EVP_MD_size(EVP_sha256()));  // 32 bytes for SHA-256
    if (!digest) goto fail;

    if (EVP_DigestFinal_ex(mdctx, digest, &digest_len) != 1) goto fail;

    *out_len = digest_len;
    EVP_MD_CTX_free(mdctx);
    return digest;

fail:
    if (mdctx) EVP_MD_CTX_free(mdctx);
    if (digest) free(digest);
    fprintf(stderr, "hashKey: failed to hash.\n");
    return NULL;
}

unsigned char *aes256cbc_decrypt(const unsigned char *keyHash, const unsigned char *ciphertext, int ciphertext_len, int *plaintext_len_out) {
    if (ciphertext_len <= 16) {
        fprintf(stderr, "Ciphertext too short (must include 16-byte IV)\n");
        return NULL;
    }

    const unsigned char *iv = ciphertext;       // IV is first 16 bytes
    const unsigned char *ct = ciphertext + 16;  // Ciphertext after IV
    int ct_len = ciphertext_len - 16;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        fprintf(stderr, "Failed to create EVP_CIPHER_CTX\n");
        return NULL;
    }

    unsigned char *plaintext = malloc(ct_len + EVP_MAX_BLOCK_LENGTH);
    if (!plaintext) {
        EVP_CIPHER_CTX_free(ctx);
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    int len = 0;
    int plaintext_len = 0;

    // Initialize decryption context
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, keyHash, iv)) {
        fprintf(stderr, "EVP_DecryptInit_ex failed\n");
        EVP_CIPHER_CTX_free(ctx);
        free(plaintext);
        return NULL;
    }

    // Decrypt ciphertext
    if (1 != EVP_DecryptUpdate(ctx, plaintext, &len, ct, ct_len)) {
        fprintf(stderr, "EVP_DecryptUpdate failed\n");
        EVP_CIPHER_CTX_free(ctx);
        free(plaintext);
        return NULL;
    }
    plaintext_len = len;

    // Finalize decryption (handle padding)
    if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) {
        fprintf(stderr, "EVP_DecryptFinal_ex failed (bad padding or key)\n");
        EVP_CIPHER_CTX_free(ctx);
        free(plaintext);
        return NULL;
    }
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    *plaintext_len_out = plaintext_len;
    return plaintext;  // caller must free()
}