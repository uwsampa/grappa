
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __GET_PUT_H__
#define __GET_PUT_H__

#include "global_array.h"
#include "thread.h"

typedef struct mem_tag_t {
    void* dest; // dest for local, NULL for remote
    void* src;  // src for local, nbhandle for remote
} mem_tag_t;


bool isLocal(global_array* ga, uint64_t index) ;
void get_remote(global_array* ga, uint64_t index, uint64_t offset, size_t size, void* dest) ;
void put_remote(global_array* ga, uint64_t index, void* data, uint64_t offset, size_t size) ;
void put_remote_nbi(global_array* ga, uint64_t index, void* data, uint64_t offset, size_t size) ;
void put_remote_address(global_address gaddr, void* data, uint64_t offset, size_t size) ;
void put_remote_address_nbi(global_address gaddr, void* data, uint64_t offset, size_t size) ;
void sync_nbi() ;
mem_tag_t get_doublelong_nb(global_array* ga, uint64_t index, void* dest) ;
mem_tag_t get_remote_nb(global_array* ga, uint64_t index, size_t num_bytes, void* dest) ;
void complete_nb(thread* me, mem_tag_t tag) ;



#endif


#ifdef __cplusplus
}
#endif
