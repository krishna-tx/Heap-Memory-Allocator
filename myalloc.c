#include <unistd.h>
#include "myalloc.h"

void *myalloc(int size) {
    if(size < 1) { return NULL; }
    int required_size = size + sizeof(BlockHeader);
    int remainder = required_size & 7;
    if(remainder != 0) { required_size += 8 - remainder; } // add padding size

    BlockHeader *current = heap_start;
    BlockHeader *best = NULL;
    BlockHeader *ftr  = NULL;
    int block_size, block_hdr;
    int best_size = -1, best_hdr = -1;
    while(current->size_status != 1) { // perform search for block using best-fit policy
        block_hdr = current->size_status;
        block_size = block_hdr;
        if(block_size & 2) { block_size -= 2; } // subtract p-bit
        if((block_size & 1) == 0) { // check if block is free
            if(block_size == required_size) { // check if found exact fit
                best_size = block_size;
                best_hdr = block_hdr;
                best = current;
                break;
            } 
            else if(block_size > required_size) {
                if(best_size == -1 || block_size < best_size) {
                    best_size = block_size;
                    best_hdr = block_hdr;
                    best = current;
                }
            }
        } 
        else { block_size -= 1; } // block is already allocated
        current = (BlockHeader *)((void *)current + block_size); // move to next header
    }

    if(best == NULL) { return NULL; } // allocation failed

    // split the block if big enough
    if(best_size - required_size) {
        BlockHeader *new_block = (BlockHeader *)((void *)best + required_size); // create new block
        new_block->size_status = best_size - required_size; // set new HDR
        if(best_size - required_size >= 4) {
            ftr = (BlockHeader *)((void *)best + best_size - sizeof(BlockHeader));
            ftr->size_status = best_size - required_size; // change FTR
        }
    }

    // set p-bit of next block
    BlockHeader *next_block = (BlockHeader *)((void *)best + required_size);
    if(next_block->size_status != 1) { next_block->size_status += 2; }

    best->size_status = required_size; // change HDR
    if(best_hdr & 2) { best->size_status += 2; } // add p-bit back
    best->size_status += 1; // set a-bit

    void *ptr = (void *)best + sizeof(BlockHeader); // payload ptr
    return ptr;
}

int myfree(void *ptr) {    
    if(ptr == NULL) { return -1; } // ptr is NULL
    
    uintptr_t ptr_addr = (uintptr_t)ptr;
    if(ptr_addr & 7) { return -1; } // ptr addr is not aligned to 8 bytes

    // find the end of the allocated heap memory
    BlockHeader *current = heap_start;
    int current_size;
    while(current->size_status != 1) {
        current_size = current->size_status;
        if(current_size & 2) { current_size -= 2; }
        if(current_size & 1) { current_size -= 1; }
        current = (BlockHeader *)((void *)current + current_size);
    }
    
    uintptr_t heap_start_addr = (uintptr_t)heap_start;
    uintptr_t heap_end_addr = (uintptr_t)current;
    if(ptr_addr < heap_start_addr || ptr_addr > heap_end_addr) { return -1; } // ptr out of range

    BlockHeader *block = (BlockHeader *)((void *)ptr - sizeof(BlockHeader));
    int block_hdr = block->size_status;
    if((block_hdr & 1) == 0) { return -1; } // block is already free

    // get block size
    int block_size = block_hdr;
    if(block_size & 2) { block_size -= 2; }
    block_size -= 1;

    // unset p-bit of next block
    BlockHeader *next_block = (BlockHeader *)((void *)block + block_size);
    if(next_block->size_status != 1) { next_block->size_status -= 2; }

    // check if next block is free
    if((next_block->size_status & 1) == 0) {
        int next_block_size = next_block->size_status;
        block_size += next_block_size; // immediate coalescing
    }

    // check if prev block is free
    if((block->size_status & 2) == 0) {
        BlockHeader *prev_block_ftr = (BlockHeader *)((void *)block - sizeof(BlockHeader));
        block_size += prev_block_ftr->size_status; // immediate coalescing
        block = (BlockHeader *)((void *)block - prev_block_ftr->size_status);
        block_hdr = block->size_status;
    }

    // change block HDR
    block->size_status = block_size;
    if(block_hdr & 2) { block->size_status += 2; }

    // change block FTR
    BlockHeader *ftr = (BlockHeader * )((void *)block + block_size - sizeof(BlockHeader));
    ftr->size_status = block_size;

    return 0; // freeing successful
}