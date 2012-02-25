#include "global_array.h"
#include "SoftXMT.hpp"

static uint64_t volatile results;
static struct global_array *current = NULL;

struct ga_request_args {
    Node from;
    uint64_t comp_addr;
};

void ga_handler_am( ga_request_args * args, size_t size, void * payload, size_t payload_size ) {
    current->component_addresses[args->from].offset = args->comp_addr;
    current->component_addresses[args->from].node = args->from;

    atomic_fetch_and_add_uint64(&results, 1);    
}
       
struct global_array *ga_allocate( int element_size, uint64_t  size ) {
    struct global_array *ga;

    ga = (global_array*) malloc(sizeof(*ga));
    ga->element_size = element_size;
    ga->size = size;
    ga->elements_per_node = size / gasnet_nodes();
    ga->allocated_size = ga->elements_per_node * gasnet_nodes();
    ga->component_addresses = (global_address*)malloc(sizeof(struct global_address) * gasnet_nodes());

    current = ga;
    results = 0;

    SoftXMT_barrier_commsafe(); 

    gm_allocate(&ga->component_addresses[gasnet_mynode()], gasnet_mynode(),
            ga->elements_per_node * ga->element_size);

    // communicate my location to everyone else
    for (Node i = 0; i < SoftXMT_nodes(); i++) {
        if (i != SoftXMT_mynode()) {
            ga_request_args gargs = { SoftXMT_mynode(), ga->component_addresses[gasnet_mynode()].offset };
            SoftXMT_call_on( i, &ga_handler_am, &gargs );
        }
    }
    while (results < (SoftXMT_nodes() - 1)) {
        address_expose();
        SoftXMT_yield();/*gasnet_AMPoll();*/
    }

    current = NULL;
    return ga;
}

    
