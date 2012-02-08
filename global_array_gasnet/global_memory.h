
#ifdef __cplusplus
extern "C" {
#endif


#ifndef _GLOBAL_MEMORY_H_
#define _GLOBAL_MEMORY_H_

#include <mpi.h>
#include <gasnet.h>
#include "base.h"

/*
    "Global" memory "allocator".
    
    * Remote allocation is not thread safe.  Other aspects of this module are.
    
    * Note: that allocations may not span nodes.  An additional abstraction layer
      can provide that.
      
    * freeing objects is not supported.
    
 */

typedef struct global_address {
    int node;
    uint64_t offset;
} global_address;


static inline int gm_node_of_address(struct global_address *ga) {
    return ga->node;
    }
    
#define ANY_NODE    -1
void gm_allocate(struct global_address *ga, int preferred_node, uint64_t size);
void gm_copy_from(struct global_address *ga, void *local_addr, uint64_t size);
void gm_copy_to(void *local_addr, struct global_address *ga, uint64_t size);


/*static inline*/ void *gm_local_gm_address_to_local_ptr(struct global_address *ga);

// Warning: pointer may not be valid on the machine that calls this!
void *gm_address_to_ptr(struct global_address *ga);

static inline int gm_is_address_local(struct global_address *ga) {
    if (ga->node == gasnet_mynode())
        return true;
    return false;
    }

void gm_init();

#define GM_REQUEST_HANDLER  129
#define GM_RESPONSE_HANDLER 130

void gm_request_handler(gasnet_token_t token,
    gasnet_handlerarg_t a0, gasnet_handlerarg_t a1);
void gm_response_handler(gasnet_token_t token,
    gasnet_handlerarg_t a0, gasnet_handlerarg_t a1);


#endif // _GLOBAL_MEMORY_H_



#ifdef __cplusplus
}
#endif
