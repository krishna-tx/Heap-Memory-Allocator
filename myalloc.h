#ifndef MYALLOC_H
#define MYALLOC_H

typedef struct BlockHeader {
    int size_status;
} BlockHeader;

typedef struct Pointer {
    int numb;
    void *ptr;
} Pointer;

extern BlockHeader *heap_start;

void *myalloc(int size);
int myfree(void *ptr);

#endif