#include "balloc.h"
#include <stdlib.h>

ballocator_t* newBumpAllocator(void* start, size_t size) {
    ballocator_t* res = (ballocator_t*)malloc(sizeof(ballocator_t));
    res->next = start;
    res->current_size = 0;
    res->total_size = size;
    return res;
}

void* balloc(ballocator_t* b, size_t size) {
    assert(b->current_size+size <= b->total_size);
    void* result = b->next;
    b->next = ((char*)b->next) + size;
    return result;
}

