#include "global_memory.h"
#include "glog/logging.h"
#include "SoftXMT.hpp"

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
        
        CHECK (!((response + request) > shared_memory_blocks[gasnet_mynode()].size)) 
               << "response(" << response << ") +"
               << "request(" << request << ") > "
               << "size(" << shared_memory_blocks[gasnet_mynode()].size << ")";

        response = 0;
        }
    return response;
}

struct gm_request_handler_args {
    Node from;
    uint64_t request;
};

struct gm_response_handler_args {
    uint64_t response;
};

void gm_response_handler_am( gm_response_handler_args * args, size_t size, void * payload, size_t payload_size) {
    response_value = args->response;
    response_received = true;
}

void gm_request_handler_am( gm_request_handler_args * args, size_t size, void * payload, size_t payload_size ) {
   uint64_t response = _perform_local_allocation(args->request);

   gm_response_handler_args resp_args = { response };
  
   SoftXMT_call_on( args->from, &gm_response_handler_am, &resp_args);
}   


void gm_allocate(struct global_address *a, int preferred_node, uint64_t size) {
    if (preferred_node == ANY_NODE) {
        preferred_node = next_random_node;
        next_random_node = next_random_node + 1;
        if (next_random_node >= gasnet_nodes())
            next_random_node = 0;
    }
    if (preferred_node == SoftXMT_mynode()) {
        // yeah!  local node
        response_value = _perform_local_allocation(size);
    } else {
        response_received = false;
        gm_request_handler_args req_args = { SoftXMT_mynode(), size };
        SoftXMT_call_on( preferred_node, &gm_request_handler_am, &req_args );
        while (!response_received) { //wait with delegate ops instead
            address_expose();
            SoftXMT_yield(); /* i.e. gasnet_AMPoll(); */ 
        }
    }
    
    CHECK_NE(response_value, 0) << "Out of memory? node=" << preferred_node << " size=" << size;

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

