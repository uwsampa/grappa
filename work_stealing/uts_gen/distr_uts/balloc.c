#include "balloc.h"
#include <stdlib.h>

ballocator_t* newBumpAllocator(global_array* ga, uint64_t start, size_t localSize) {
    ballocator_t* res = (ballocator_t*)malloc(sizeof(ballocator_t));
    res->next = start;
    res->current_size = 0;
    res->total_size = localSize;
    return res;
}

uint64_t balloc(ballocator_t* b, size_t size) {
    assert(b->current_size+size <= b->total_size);
    uint64_t result = b->next;
    b->next = b->next + size;
    return result;
}

