#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <stdbool.h>  // For bool

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file crypto_utils.h
 * @brief Utility functions for hashing, key validation, and AES-256-CBC decryption
 *        using OpenSSL EVP APIs.
 *
 * This module provides:
 *   - SHA-256 hashing of arbitrary strings.
 *   - Validation of a key hash against a known SHA-256 hash.
 *   - AES-256-CBC decryption of ciphertexts with IV prepended.
 */

/**
 * @brief Validate whether a given SHA-256 hash matches the expected key hash.
 *
 * Compares the provided 32-byte hash with the known SHA-256 checksum of the
 * 256-bit symmetric key used to encrypt the ciphertext.
 *
 * @param hash
 *     Pointer to a 32-byte array containing the SHA-256 hash to validate.
 *
 * @return
 *     `true` if the provided hash matches the known hash, `false` otherwise.
 */
bool validateKeyHash(unsigned char *hash);

/**
 * @brief Compute the SHA-256 hash of a plaintext key string.
 *
 * Computes the SHA-256 hash of a NUL-terminated ASCII string using the
 * OpenSSL EVP interface. The caller must free the returned buffer.
 *
 * @param plaintextKey
 *     Pointer to a NUL-terminated C string to be hashed.
 *
 * @param out_len
 *     Pointer to an unsigned int that will receive the number of bytes written
 *     to the returned digest (always 32 for SHA-256).
 *
 * @return
 *     Pointer to a newly allocated buffer containing the raw 32-byte digest.
 *     The caller is responsible for freeing this buffer with `free()`.
 *     Returns NULL on error.
 */
unsigned char *hashKey(const char *plaintextKey, int *out_len);

/**
 * @brief Decrypt AES-256-CBC ciphertext using OpenSSL EVP.
 *
 * The input buffer must contain a 16-byte initialization vector (IV)
 * prepended to the ciphertext. The function allocates a plaintext buffer
 * dynamically, which the caller must free.
 *
 * @param keyHash
 *     Pointer to a 32-byte AES key used for decryption.
 *
 * @param ciphertext
 *     Pointer to the ciphertext buffer (16-byte IV + ciphertext data).
 *
 * @param ciphertext_len
 *     Total length of the ciphertext buffer, including the 16-byte IV.
 *
 * @param plaintext_len_out
 *     Pointer to an integer that will be set to the length of the decrypted plaintext.
 *
 * @return
 *     Pointer to a dynamically allocated plaintext buffer. Caller must free it.
 *     Returns NULL on failure (invalid padding, incorrect key, or memory error).
 */
unsigned char *aes256cbc_decrypt(const unsigned char *keyHash,
                                 const unsigned char *ciphertext,
                                 int ciphertext_len,
                                 int *plaintext_len_out);

#ifdef __cplusplus
}
#endif

#endif  // CRYPTO_UTILS_H
