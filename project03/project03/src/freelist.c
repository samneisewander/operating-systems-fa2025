/* freelist.c: Free List Implementation
 *
 * The FreeList is an unordered doubly-linked circular list containing all the
 * available memory allocations (memory that has been previous allocated and
 * can be re-used).
 **/

#include "malloc/counters.h"
#include "malloc/freelist.h"

/* Global Variables */

Block FreeList = {0, 0, &FreeList, &FreeList};

/* Functions */

/**
 * Search for an existing block in free list with at least the specified size
 * using the first fit algorithm.
 * @param   size    Amount of memory required.
 * @return  Pointer to existing block (otherwise NULL if none are available).
 **/
Block * free_list_search_ff(size_t size) {
    for (Block *curr = FreeList.next; curr != &FreeList; curr = curr->next){
        if (curr->capacity >= size){
            curr->size = size;
            return curr;
        }
    }

    return NULL;
}

/**
 * Search for an existing block in free list with at least the specified size
 * using the best fit algorithm.
 * @param   size    Amount of memory required.
 * @return  Pointer to existing block (otherwise NULL if none are available).
 **/
Block * free_list_search_bf(size_t size) {
    Block *best = NULL;
    for (Block *curr = FreeList.next; curr != &FreeList; curr = curr->next){
        if (curr->capacity >= size){
            if (!best || (best->capacity > curr->capacity)){
                best = curr;
            }
        }
    }
    if (best){     
        best->size = size;
    }
    return best;
}

/**
 * Search for an existing block in free list with at least the specified size
 * using the worst fit algorithm.
 * @param   size    Amount of memory required.
 * @return  Pointer to existing block (otherwise NULL if none are available).
 **/
Block * free_list_search_wf(size_t size) {
    Block *worst = NULL;
    for (Block *curr = FreeList.next; curr != &FreeList; curr = curr->next){
        if (curr->capacity >= size){
            if (!worst || (worst->capacity < curr->capacity)){
                worst = curr;
            }
        }
    }

    if (worst){
        worst->size = size;
    }
    return worst;
}

/**
 * Search for an existing block in free list with at least the specified size.
 *
 * Note, this is a wrapper function that calls one of the three algorithms
 * above based on the compile-time setting.
 *
 * @param   size    Amount of memory required.
 * @return  Pointer to existing block (otherwise NULL if none are available).
 **/
Block * free_list_search(size_t size) {
    Block * block = NULL;
#if     defined FIT && FIT == 0
    block = free_list_search_ff(size);
#elif   defined FIT && FIT == 1
    block = free_list_search_wf(size);
#elif   defined FIT && FIT == 2
    block = free_list_search_bf(size);
#endif

    // Update Counters
    if (block) {
        Counters[REUSES]++;
    }
    return block;
}

/**
 * Insert specified block into free list.
 *
 * Scan the free list for the first block whose address is after the given
 * input block and then insert the block into the free list in a manner that
 * ensures that the blocks in the free list are stored in ascending address
 * values (ie. the first block's address is less than the second block's
 * address, etc.).
 *
 * After inserting the block into the free list, attempt to merge block with
 * adjacent blocks.
 *
 * @param   block   Pointer to block to insert into free list.
 **/
void    free_list_insert(Block *block) {
    if (!block) return;

    // Insert the block into the free list such that it is in sorted order by block address
    Block *curr = NULL;
    for (curr = FreeList.next; curr != &FreeList; curr = curr->next){
        if (curr > block) break;
    }

    // Update next and prev pointer of block and its neighbors
    block->next = curr; 
    block->prev = curr->prev;
    curr->prev->next = block;
    curr->prev = block;

    // Attempt to merge block with adjacent blocks
    block_merge(block, block->next);
    block_merge(block->prev, block);
}

/**
 * Return length of free list.
 * @return  Length of the free list.
 **/
size_t  free_list_length() {
    // Implement free list length
    size_t count = 0;
    for (Block *curr = FreeList.next; curr != &FreeList; curr = curr->next) {
        count++;
    }
    return count;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
