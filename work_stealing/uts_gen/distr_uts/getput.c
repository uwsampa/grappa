#include "getput.h"
#include <gasnet.h>

bool isLocal(global_array* ga, uint64_t index) {
    global_address gaddr;
    ga_index(ga, index, &gaddr);
    return gm_is_address_local(&gaddr);
}

void get_remote(global_array* ga, uint64_t index, uint64_t offset, size_t size, void* dest) {
    global_address gaddr;
    ga_index(ga, index, &gaddr);
    gaddr.offset += offset;
    if (gm_is_address_local(&gaddr)) {
        void* v = (uint64_t*) gm_local_gm_address_to_local_ptr(&gaddr);
        memcpy(dest, v, size);
    } else {
        void* srcptr = gm_address_to_ptr(&gaddr);
        int nodeid = gm_node_of_address(&gaddr);
        gasnet_get (dest, nodeid, srcptr, size);
    }
}

void put_remote(global_array* ga, uint64_t index, void* data, uint64_t offset, size_t size) {
    global_address gaddr;
    //printf("node%d puts at index=%lu\n", gasnet_mynode(), index);
    ga_index(ga, index, &gaddr);
    gaddr.offset += offset;
    if (gm_is_address_local(&gaddr)) {
        void* v = (uint64_t*) gm_local_gm_address_to_local_ptr(&gaddr);
        memcpy(v, data, size);
 //       printf("%p: data=%lu %lu, size=%lu\n", v, ((uint64_t*)data)[0], ((uint64_t*)data)[1], size);
    } else {
        void* destptr = gm_address_to_ptr(&gaddr);
        int nodeid = gm_node_of_address(&gaddr);
        //printf("node%d puts <%d, %p, %lu, %lu>\n", gasnet_mynode(), nodeid, destptr, data, size);
        gasnet_put (nodeid, destptr, data, size);
    }
}

void put_remote_address(global_address gaddr, void* data, uint64_t offset, size_t size) {
    gaddr.offset += offset;
    if (gm_is_address_local(&gaddr)) {
        void* v = (uint64_t*) gm_local_gm_address_to_local_ptr(&gaddr);
        memcpy(v, data, size);
 //       printf("%p: data=%lu %lu, size=%lu\n", v, ((uint64_t*)data)[0], ((uint64_t*)data)[1], size);
    } else {
        void* destptr = gm_address_to_ptr(&gaddr);
        int nodeid = gm_node_of_address(&gaddr);
        //printf("node%d puts <%d, %p, %lu, %lu>\n", gasnet_mynode(), nodeid, destptr, data, size);
        gasnet_put (nodeid, destptr, data, size);
    }
}

#define PRE_LOCALITY 1
#define prefetchDL(src) __builtin_prefetch((src), 0, PRE_LOCALITY); \
                        __builtin_prefetch(((uint8_t*)(src))+8, 0, PRE_LOCALITY)

mem_tag_t get_doublelong_nb(global_array* ga, uint64_t index, void* dest) {
    global_address gaddr;
    ga_index(ga, index, &gaddr);
    void* srcptr;
    int nodeid;
    if (gm_is_address_local(&gaddr)) {
        //printf("p%d: local for index %d\n", gasnet_mynode(), index);
        srcptr = gm_local_gm_address_to_local_ptr(&gaddr);
       prefetchDL(srcptr);
        mem_tag_t tag;
        tag.dest = dest;
        tag.src = srcptr;
        return tag;
    } else {
        //printf("p%d: remote for index %d\n", gasnet_mynode(), index);
        srcptr = gm_address_to_ptr(&gaddr);
        nodeid = gm_node_of_address(&gaddr);
        mem_tag_t tag;
        tag.dest = NULL;
        tag.src = gasnet_get_nb (dest, nodeid, srcptr, 2*sizeof(uint64_t));
        return tag;
    }
}


/*
gasnet_handle_t put_long_nb(global_array* ga, uint64_t index, uint64_t data) {
    global_address gaddr;
    ga_index(ga, index, &gaddr);
    void* destptr;
    int nodeid;
    if (gm_is_address_local(&gaddr)) {
        destptr = gm_local_gm_address_to_local_ptr(&gaddr);
        nodeid = gasnet_mynode();
    } else {
        destptr = gm_address_to_ptr(&gaddr);
        nodeid = gm_node_of_address(&gaddr);
    }
    
    return gasnet_put_nb (nodeid, destptr, &data, sizeof(uint64_t));
}
*/
