/* block.c: Block Structure */

#include "malloc/block.h"
#include "malloc/counters.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

/**
 * Allocate a new block on the heap using sbrk:
 *
 *  1. Determined aligned amount of memory to allocate.
 *  2. Allocate memory on the heap.
 *  3. Set allocated block properties.
 *
 * @param   size    Number of bytes to allocate.
 * @return  Pointer to data portion of newly allocate block.
 **/
Block * block_allocate(size_t size) {
    // Allocate block
    intptr_t allocated = sizeof(Block) + ALIGN(size);
    Block *  block     = sbrk(allocated);
    if (block == SBRK_FAILURE) {
        return NULL;
    }

    // Record block information
    block->capacity = ALIGN(size);
    block->size     = size;
    block->prev     = block;
    block->next     = block;

    // Update counters
    Counters[HEAP_SIZE] += allocated;
    Counters[BLOCKS]++;
    Counters[GROWS]++;
    return block;
}

/**
 * Attempt to release memory used by block to heap:
 *
 *  1. If the block is at the end of the heap.
 *  2. The block allocation meets the trim threshold.
 *
 * @param   block   Pointer to block to release.
 * @return  Whether or not the release completed successfully.
 **/
bool    block_release(Block *block) {
    if (!block) return false;
    size_t allocated = block->capacity + sizeof(Block);

    if ((block->data + block->capacity) == sbrk(0) && block->capacity >= TRIM_THRESHOLD) {
        block = block_detach(block);
        Block* status = sbrk(-allocated);
        if (status == SBRK_FAILURE) {
            fprintf(stderr, "block_release: failed to deallocate block.\n");
            return false;
        } 
        // Update counters
        Counters[BLOCKS]--;
        Counters[SHRINKS]++;
        Counters[HEAP_SIZE] -= allocated;

        return true;
    } 

    return false;
}

/**
 * Detach specified block from its neighbors.
 *
 * @param   block   Pointer to block to detach.
 * @return  Pointer to detached block.
 **/
Block * block_detach(Block *block) {
    // Detach block from neighbors by updating previous and next block
    if (!block) return NULL;

    if (block->prev) {
        block->prev->next = block->next;
    }

    if (block->next) {
        block->next->prev = block->prev;
    }

    block->next = block;
    block->prev = block;

    return block;
}

/**
 * Attempt to merge source block into destination.
 *
 *  1. Compute end of destination and start of source.
 *
 *  2. If they both match, then merge source into destination by giving the
 *  destination all of the memory allocated to source.
 *
 *  3. Update references from and to destination block appropriately.
 *
 * @param   dst     Destination block we are merging into.
 * @param   src     Source block we are merging from.
 * @return  Whether or not the merge completed successfully.
 **/
bool    block_merge(Block *dst, Block *src) {
    if (!dst || !src) return false;
    
    // Compare end of dst to start of src
    Block *addr = (Block *)(dst->data + dst->capacity);
    if (addr != src) return false;

    // Merge by modifying appropriate dst and src attributes
    dst->next = src->next;
    src->next->prev = dst;
    dst->capacity += sizeof(Block) + src->capacity;

    // Update Counters
    Counters[MERGES]++;
    Counters[BLOCKS]--;

    return true;
}

/**
 * Attempt to split block with the specified size:
 *
 *  1. Check if block capacity is sufficient for requested aligned size and
 *  header Block.
 *
 *  2. Split specified block into two blocks.
 *
 * Note: Set block size to specified size even if a split does not occur.
 *
 * @param   block   Pointer to block to split into two separate blocks.
 * @param   size    Desired size of the first block after split.
 * @return  Pointer to original block (regardless if it was split or not).
 **/
Block * block_split(Block *block, size_t size) {
    // Check if block is valid and has enough capacity
    if (!block) return block;    
    
    block->size = size;
    if (block->capacity <= (ALIGN(size) + sizeof(Block))) return block;
    
    // Split block by imposing new block and updating appropriate attributes of original and new block
    size_t old_capacity = block->capacity;
    block->capacity = ALIGN(size);
    Block *new_obj = (Block *)(block->data + block->capacity);
    new_obj->capacity = old_capacity - block->capacity - sizeof(Block);
    new_obj->size = 0;

    // Update ptrs
    new_obj->next = block->next;
    block->next->prev = new_obj;
    block->next = new_obj;
    new_obj->prev = block;

    // Update Counters
    Counters[SPLITS]++;
    Counters[BLOCKS]++;

    return block;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
