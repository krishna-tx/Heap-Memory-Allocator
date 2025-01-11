# Heap-Memory-Allocator

myalloc.c contains the source code where the myalloc() and myfree() functions are implemented. These functions are modeled after the behavior of malloc() and free() for manipulating heap memory in process space. They use the Best-Fit Placement Policy for picking an appropriate block size of memory to allocate and implement Immediate Coalescing to join blocks upon freeing. 

interactive.c contains the source code for an interactive tool that is used to test the implementation of the myalloc() and myfree() functions. 
