#include <stdio.h>

#include "smq/crypto.h"

int main() {
    int outLen;
    unsigned char *hash = hashKey("fish", &outLen);

    FILE *out = fopen("keyhash.bin", "w");
    if (!out) return 1;

    fwrite(hash, sizeof(unsigned char), outLen, out);

    free(hash);
}