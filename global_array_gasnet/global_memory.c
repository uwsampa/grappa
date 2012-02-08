#include "global_memory.h"


gasnet_seginfo_t    *shared_memory_blocks;
void *my_shared_memory_block; 

// "0" is an error respnse
uint64_t local_allocation_offset = sizeof(uint64_t);
    
void gm_init() {
    shared_memory_blocks = (gasnet_seginfo_t*)malloc(sizeof(gasnet_seginfo_t) * gasnet_nodes());
    if (gasnet_getSegmentInfo(shared_memory_blocks, gasnet_nodes()) !=
        GASNET_OK) {
        printf("Failed to inquire about shared memory\n");
        gasnet_exit(1);
        }
    my_shared_memory_block = shared_memory_blocks[gasnet_mynode()].addr;
    }

static volatile int response_received; /* bool */
static uint64_t response_value;
static int next_random_node = 0;


/*static inline*/ void *gm_local_gm_address_to_local_ptr(struct global_address *ga) {
    assert(ga->node == gasnet_mynode());
    return (void *)((uint8_t *)my_shared_memory_block + ga->offset);
    }


static uint64_t _perform_local_allocation(uint64_t request) {
    uint64_t response;
    
    response = atomic_fetch_and_add_uint64(&local_allocation_offset, request);
    if ((response + request) >
        shared_memory_blocks[gasnet_mynode()].size) {
        // Out of memory!
        response = 0;
        }
    return response;
}

void gm_request_handler(gasnet_token_t token,
    gasnet_handlerarg_t a0,
    gasnet_handlerarg_t a1) {
    uint64_t request, response;
    
    request = a0;
    request = request << 32;
    request = request | a1;
    
    response = _perform_local_allocation(request);
    
    gasnet_AMReplyShort2(token, GM_RESPONSE_HANDLER,
        (response >> 32), (response & 0xffffffff));
    }

void gm_response_handler(gasnet_token_t token,
    gasnet_handlerarg_t a0,
    gasnet_handlerarg_t a1) {
    
    response_value = a0;
    response_value = response_value << 32;
    response_value = response_value | a1;
    
    response_received = true;
    }

void gm_allocate(struct global_address *a, int preferred_node, uint64_t size) {
    if (preferred_node == ANY_NODE) {
        preferred_node = next_random_node;
        next_random_node = next_random_node + 1;
        if (next_random_node >= gasnet_nodes())
            next_random_node = 0;
        }
    if (preferred_node == gasnet_mynode()) {
        // yeah!  local node
        response_value = _perform_local_allocation(size);
        } else {
            response_received = false;
            gasnet_AMRequestShort2(preferred_node, GM_REQUEST_HANDLER,
                (size >> 32), (size & 0xffffffff));
            while (!response_received) {
                address_expose();
                gasnet_AMPoll();
                }
        }
    if (response_value == 0)
       panic("Out of memory");
    
    a->node = preferred_node;
    a->offset = response_value;
    }

// Warning: pointer may not be valid on this machine.
void *gm_address_to_ptr(struct global_address *ga) {
    return (void *) ((uint8_t*)shared_memory_blocks[ga->node].addr + ga->offset);
    }
    
void gm_copy_from(struct global_address *ga, void *local_address, uint64_t size) {
    gasnet_get_nbi_bulk(local_address, ga->node,
        gm_address_to_ptr(ga),
        (size_t) size);
}

void gm_copy_to(void *local_address, struct global_address *ga, uint64_t size) {
    gasnet_put_nbi_bulk(ga->node,
        gm_address_to_ptr(ga),
        local_address, size);
}

