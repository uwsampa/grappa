#ifndef _BALLOC_H_
#define _BALLOC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <assert.h>

typedef struct ballocator {
    uint64_t next;
    global_array* pool;
    size_t current_size;
    size_t total_size;  // number of elements, not bytes
} ballocator_t;

ballocator_t* newBumpAllocator(global_array* ga, uint64_t start, size_t localSize);
uint64_t balloc(ballocator_t* b, size_t size);


#ifdef __cplusplus
}
#endif

#endif
