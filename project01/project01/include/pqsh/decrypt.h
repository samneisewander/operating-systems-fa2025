#include <stdio.h>
#include <stdlib.h>
#include <openssl/evp.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>


#define KEY_BITS 128
#define IV_BYTES (KEY_BITS / 8)
#define KEY_BYTES (KEY_BITS / 8)

/**
 * What could this do?
 */
int decrypt_egg();