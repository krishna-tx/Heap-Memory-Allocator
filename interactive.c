#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include "myalloc.h"

// declare global vars
BlockHeader *heap_start;  
Pointer *pointers;
int pointers_len;
int pointers_filled;
int pointers_counter;

int init_heap(int heap_size) {
    if(heap_size < 12 || (heap_size & (sizeof(BlockHeader)-1)) != 0) return -1; // heap size is too small or not a multiple of sizeof(BlockHeader)-1 for alignment
    
    // initialize heap with heap_size
    heap_start = mmap(NULL, heap_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(heap_start == MAP_FAILED) return -1; // mmap() fail
    heap_start += 1; // move heap_start by sizeof(BlockHeader) for alignment

    heap_size -= sizeof(BlockHeader) * 2; // heap size reduces due to alignment and end mark

    // set end mark to 1
    BlockHeader *heap_end = heap_start + heap_size / sizeof(BlockHeader);
    heap_end->size_status = 1;
    
    heap_start->size_status = heap_size + 2; // set HDR
    (heap_end - 1)->size_status = heap_size; // set FTR

    return 0; // success
}

void print_heap() {
    printf("****************************************\n");
    printf("Count\tBlock Status\tBlock Size\n");
    BlockHeader *current = heap_start;
    int count = 1;
    int block_size;
    char block_status[10];
    while(current->size_status != 1) {
        block_size = current->size_status;

        strcpy(block_status, "Free     ");
        if(block_size & 1) { // check if current block is allocated
            strcpy(block_status, "Allocated");
            block_size -= 1; // subtract a-bit;
        }
        if(block_size & 2) { // check if previous block is allocated
            block_size -= 2; // subtract p-bit;
        }

        printf("%i\t%s\t%i\n", count, block_status, block_size);
        count++;
        current = (BlockHeader *)((void *)current + block_size);
    }
    printf("****************************************\n");
}

void print_pointers() {
    printf("****************************************\n");
    printf("Pointer\tBlock Size\n");
    int block_size;
    for(int i = 0; i < pointers_len; i++) {
        if(pointers[i].numb > 0) { // allocated pointer
            block_size = *(int *)(pointers[i].ptr - sizeof(BlockHeader));
            if(block_size & 1) block_size -= 1;
            if(block_size & 2) block_size -= 2;
            printf("p%i\t%i\n", pointers[i].numb, block_size);
        }
    }
    printf("****************************************\n");
}

int check_numb_args() {
    char *word = strtok(NULL, " ");
    if(word != NULL) {
        printf("Invalid Number of Args!\n");
        return -1;
    }
    return 0;
}

int allocate_helper(int size) {
    // check if there is space for the new pointer
    if(pointers_filled == pointers_len) { // need to reallocate memory
        pointers_len += 5;
        pointers = realloc(pointers, sizeof(Pointer) * pointers_len);
        if(pointers == NULL) {
            printf("realloc() failed!\n");
            exit(1);
        }
        // initialize new cells to be NULL
        memset(pointers + (pointers_len - 5), 0, sizeof(void *) * 5);
    }

    // loop through pointers to find an empty slot to store the newly allocated pointer
    for(int i = 0; i < pointers_len; i++) {
        if(pointers[i].numb == 0) { // empty slot found
            void *ptr = myalloc(size);
            if(ptr == NULL) return -1; // myalloc() fail

            // myalloc() success
            pointers[i].numb = pointers_counter;
            pointers[i].ptr = ptr;
            printf("Allocated p%i\n", pointers_counter);
            pointers_filled++;
            pointers_counter++;
            break;
        }
    }
    return 0; // success
}

int free_helper(int pointer_numb) {
    // check if pointer_numb is in valid range
    if(pointer_numb <= 0 || pointer_numb >= pointers_counter) return -2;
    for(int i = 0 ; i < pointers_len; i++) {
        if(pointers[i].numb == pointer_numb) { // found pointer to free
            if(myfree(pointers[i].ptr) == -1) return -1; // myfree() fail

            // myfree() succcess
            printf("Freed p%i\n", pointers[i].numb);
            pointers[i].numb = 0; // set slot to empty
            pointers_filled--;
            return 0; // success
        }
    }
    return -2; // pointer not found
}

int main(int argc, char *argv[]) {
    // initialize global vars
    pointers_counter = 1;
    pointers_len = 5;
    pointers = calloc(pointers_len, sizeof(Pointer));
    if(pointers == NULL) {
        printf("calloc() failed!\n");
        exit(1);
    }

    // initialize a heap with the size of a page (default 4kb) to be used to simulate myalloc() and myfree()
    if(init_heap(getpagesize()) == -1) {
        printf("init_heap() failed!\n");
        return -1;
    }

    // print instructions
    char *instructions = "Type in one of the following valid commands with the proper syntax:\n"
                         "\tmyalloc [size] (Ex: \"myalloc 12\" will call myalloc(12) and allocate 12 bytes (plus a few bytes to properly align heap blocks)) - returned pointer is stored internally\n"
                         "\tmyfree [pointer#] (Ex: \"myfree 3\" will call myfree(p3) and free p3)\n"
                         "\tprint_heap - prints all the blocks on the heap\n"
                         "\tprint_pointers - prints all the allocated pointers and the size of their allocated block on the heap\n"
                         "\texit - exits this interactive program\n";
    printf("%s", instructions);

    char line[100];
    printf("> ");
    while(fgets(line, sizeof(line), stdin)) {
        // remove newline char from line
        int line_len = strlen(line);
        if(line[line_len-1] == '\n') {
            line[line_len-1] = '\0';
        }

        char *word = strtok(line, " ");
        if(word == NULL) {
            printf("> ");
            continue;
        }
        if(strcmp(word, "myalloc") == 0) {
            word = strtok(NULL, " ");
        
            // not enough args
            if(word == NULL) {
                printf("Invalid Number of Args!\n");
                printf("> ");
                continue;
            }

            // too many args
            if(check_numb_args() != 0) {
                printf("> ");
                continue;
            }

            int size = atoi(word);
            if(size <= 0) { // check if size is a positive integer
                printf("Byte size has to be a positive integer!\n");
                printf("> ");
                continue;
            }

            if(allocate_helper(size) == -1) { // perform allocation
                printf("myalloc() could not allocate block for given size!\n");
                printf("> ");
                continue;
            }
        }
        else if(strcmp(word, "myfree") == 0) {
            word = strtok(NULL, " ");
        
            // not enough args
            if(word == NULL) {
                printf("Invalid Number of Args!\n");
                printf("> ");
                continue;
            }

            // too many args
            if(check_numb_args() != 0) {
                printf("> ");
                continue;
            }
            
            int pointer_numb = atoi(word);
            int return_code = free_helper(pointer_numb); // perform freeing
            if(return_code == -1) { 
                printf("myfree() could not free the pointer!\n");
                printf("> ");
                continue;
            }
            else if(return_code == -2) {
                printf("Pointer number is invalid!\n");
                printf("> ");
                continue;
            }
        }
        else if(strcmp(word, "print_heap") == 0) {
            if(check_numb_args() != 0) { // invalid number of args
                printf("> ");
                continue;
            }
            print_heap();
        }
        else if(strcmp(word, "print_pointers") == 0) {
            if(check_numb_args() != 0) { // invalid number of args
                printf("> ");
                continue;
            }
            print_pointers();
        }
        else if(strcmp(word, "exit") == 0) {
            if(check_numb_args() != 0) { // invalid number of args
                printf("> ");
                continue;
            }
            break;
        }
        else {
            printf("Invalid command!\n");
        }
        printf("> ");
    }
    return 0;
}