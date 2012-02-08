#ifdef __cplusplus
extern "C" {
#endif

#ifndef __GET_PUT_H__
#define __GET_PUT_H__

#include "global_array.h"

typedef struct mem_tag_t {
    void* dest; // dest for local, NULL for remote
    void* src;  // src for local, nbhandle for remote
} mem_tag_t;

bool isLocal(global_array* ga, uint64_t index);

void get_remote(global_array* ga, uint64_t index, uint64_t offset, size_t size, void* dest);

void put_remote(global_array* ga, uint64_t index, void* data, uint64_t offset, size_t size);

mem_tag_t get_doublelong_nb(global_array* ga, uint64_t index, void* dest); 

/*mem_tag_t put_long_nb(global_array* ga, uint64_t index, uint64_t data);*/

#endif


#ifdef __cplusplus
}
#endif
