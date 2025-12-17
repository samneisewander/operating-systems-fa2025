/* posix.c: POSIX API Implementation */

#include "malloc/counters.h"
#include "malloc/freelist.h"

#include <assert.h>
#include <string.h>

/**
 * Allocate specified amount memory.
 * @param   size    Amount of bytes to allocate.
 * @return  Pointer to the requested amount of memory.
 **/
void *malloc(size_t size) {
    init_counters();

    if (!size) return NULL;

    // Search free list for any available block with matching size, split and detach if found, otherwise allocate a new block
    Block *block = free_list_search(size);
    if (block){
        block = block_split(block, size);
        block = block_detach(block);
    } else {
        block = block_allocate(size);
    }

    // Could not find free block or allocate a block, so just return NULL
    if (!block) {
        return NULL;
    }

    // Check if allocated block makes sense
    assert(block->capacity >= block->size);
    assert(block->size     == size);
    assert(block->next     == block);
    assert(block->prev     == block);

    // Update Counters
    Counters[MALLOCS]++;
    Counters[REQUESTED] += size;

    // Return data address associated with block
    return block->data;
}

/**
 * Release previously allocated memory.
 * @param   ptr     Pointer to previously allocated memory.
 **/
void free(void *ptr) {
    if (!ptr) return;

    // Update Counters
    Counters[FREES]++;

    // Try to release block, otherwise insert it into the free list
    Block *block = BLOCK_FROM_POINTER(ptr);
    if (!block_release(block)){
        free_list_insert(block);
    }
}

/**
 * Allocate memory with specified number of elements and with each element set
 * to 0.
 * @param   nmemb   Number of elements.
 * @param   size    Size of each element.
 * @return  Pointer to requested amount of memory.
 **/
void *calloc(size_t nmemb, size_t size) {
    // Implement calloc using malloc and memset
    if (nmemb > 0 && size > SIZE_MAX / nmemb) {
        return NULL;
    }
    void *ptr = malloc(nmemb * size);
    if (ptr){
        memset(ptr, 0, nmemb*size);
    }

    // Update Counters
    Counters[CALLOCS]++;

    return ptr;
}

/**
 * Reallocate memory with specified size.
 * @param   ptr     Pointer to previously allocated memory.
 * @param   size    Amount of bytes to allocate.
 * @return  Pointer to requested amount of memory.
 **/
void *realloc(void *ptr, size_t size) {
    // Case 1: ptr is null, return size;
    if (!ptr){
        return malloc(size);
    }
    
    // Case 2: ptr has enough size, return it
    Block *block = BLOCK_FROM_POINTER(ptr);
    if (block->capacity >= size){
        block->size = block->size < size ? size : block->size; 
        return ptr;
    }
    
    // Case 3: need to realloc 
    void *new_ptr = malloc(size);
    if (!new_ptr) {
        return NULL; 
    }
    memcpy(new_ptr, ptr, block->size);
    free(ptr);

    // Update Counters
    Counters[REALLOCS]++;

    return new_ptr;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
