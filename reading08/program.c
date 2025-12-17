/* Reading 08 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Constants */

#define KB 1024
#define SEGMENT_MASK 0xC000
#define SEGMENT_SHIFT 14
#define OFFSET_MASK ~SEGMENT_MASK  // TODO: Define Bitmask for Offset portion of Virtual Address

/* Structures */

typedef struct {
    uint16_t base;
    uint16_t size;
} SegmentRecord;

/* Constants */

SegmentRecord MMU[] = {
    // Segmentation Table stored in MMU
    {.base = 32 * KB, .size = 2 * KB}, /* 00: Code  */
    {.base = 34 * KB, .size = 2 * KB}, /* 01: Data  */
    {.base = 36 * KB, .size = 2 * KB}, /* 10: Heap  */
    {.base = 28 * KB, .size = 2 * KB}, /* 11: Stack */
};

int main(int argc, char* argv[]) {
    uint16_t virtual_address;

    while (fread(&virtual_address, sizeof(uint16_t), 1, stdin)) {
        uint16_t segment = (virtual_address & SEGMENT_MASK) >> SEGMENT_SHIFT;
        uint16_t offset = virtual_address & OFFSET_MASK;

        char* fault = "";
        if (offset > MMU[segment].size) {
            fault = " Segmentation Fault";
        }

        uint16_t physical_address = offset + MMU[segment].base;
        printf("VA[%04hx] -> PA[%04hx]%s\n", virtual_address, physical_address, fault);
    }

    return EXIT_SUCCESS;
}