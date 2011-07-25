#include "global_memory.h"


gasnet_seginfo_t    *shared_memory_blocks;
void *my_shared_memory_block;

// "0" is an error respnse
uint64 local_allocation_offset = sizeof(uint64);
    
void gm_init() {
    shared_memory_blocks = malloc(sizeof(gasnet_seginfo_t) * gasnet_nodes());
    if (gasnet_getSegmentInfo(shared_memory_blocks, gasnet_nodes()) !=
        GASNET_OK) {
        printf("Failed to inquire about shared memory\n");
        gasnet_exit(1);
        }
    my_shared_memory_block = shared_memory_blocks[gasnet_mynode()].addr;
    }

static volatile bool response_received;
static uint64 response_value;
static int next_random_node = 0;

static uint64 _perform_local_allocation(uint64 request) {
    uint64 response;
    
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
    uint64 request, response;
    
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

int gm_allocate(struct global_address *a, int preferred_node, uint64 size) {
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
        return -1;  // FAIL
    
    a->node = preferred_node;
    a->offset = response_value;
    return 0;
    }

int gm_copy_from(struct global_address *ga, void *local_address, uint64 size) {
    gasnet_get_bulk(local_address, ga->node,
        (void *) ((uint8*)shared_memory_blocks[ga->node].addr + ga->offset),
        (size_t) size);
    return 0;
}

int gm_copy_to(void *local_address, struct global_address *ga, uint64 size) {
    gasnet_put_bulk(ga->node,
        (void *) ((uint8*)shared_memory_blocks[ga->node].addr + ga->offset),
        local_address, size);
    return 0;
}

