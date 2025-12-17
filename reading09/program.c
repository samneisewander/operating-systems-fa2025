/* Reading 09 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Constants */

#define ADDRESS_LENGTH 8    // Number of bits in virtual address
#define PAGE_SIZE (1 << 4)  // Define number of bytes per Page

#define MAX_PAGES (1 << 4)     // Define number of Pages in virtual address space
#define VPN_MASK (0xF << 4)    // Define bitmask for VPN portion of virtual address
#define VPN_SHIFT 4            // Define bitshift amount for VPN portion of virtual address
#define OFFSET_MASK ~VPN_MASK  // Define bitmask for offset portion of virtual address

/* Structures */

/**
 * Each Page Table Entry is a byte with the followin bit fields:
 *
 *  NNNNNNVP
 *
 *  NNNNNN: Physical Frame Number
 *  V:     Valid bit (page is valid/in-use)
 *  P:     Protection bit (page is protected/accessible)
 **/
typedef struct {
    uint8_t PFN : 6;
    uint8_t Valid : 1;
    uint8_t Protected : 1;
} PageTableEntry;

/* Constants */

PageTableEntry PageTable[MAX_PAGES] = {
    // Page Table (normally in physical memory)
    {.PFN = 3, .Valid = 1, .Protected = 0}, /* Page 0 */
    {.PFN = 7, .Valid = 1, .Protected = 0}, /* Page 1 */
    {.PFN = 5, .Valid = 1, .Protected = 0}, /* Page 2 */
    {.PFN = 2, .Valid = 1, .Protected = 1}, /* Page 3 */
};

int main(int argc, char* argv[]) {
    uint8_t virtual_address;

    while (fread(&virtual_address, sizeof(uint8_t), 1, stdin)) {
        uint8_t vpn = (virtual_address & VPN_MASK) >> VPN_SHIFT;
        uint8_t offset = virtual_address & OFFSET_MASK;

        char* fault = "";
        if (!PageTable[vpn].Valid) {
            fault = " Segmentation Fault";
        } else if (PageTable[vpn].Protected) {
            fault = " Protection Fault";
        }

        uint8_t physical_address = (PageTable[vpn].PFN * PAGE_SIZE) + offset;
        printf("VA[%02x] -> PA[%02x]%s\n", virtual_address, physical_address, fault);
    }

    return EXIT_SUCCESS;
}