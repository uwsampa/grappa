#ifndef _BALLOC_H_
#define _BALLOC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <assert.h>

typedef struct ballocator {
    void* next;
    size_t current_size;
    size_t total_size;
} ballocator_t;

ballocator_t* newBumpAllocator(void* start, size_t size);
void* balloc(ballocator_t* b, size_t size);


#ifdef __cplusplus
}
#endif

#endif
