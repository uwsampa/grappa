#include "global_array.h"

static uint64 volatile results;
static struct global_array *current = NULL;

void ga_handler(gasnet_token_t token,
    gasnet_handlerarg_t a0,
    gasnet_handlerarg_t a1,
    gasnet_handlerarg_t a2) {
    uint64  a;
    
    a = a1;
    a = a << 32;
    a = a | a2;
    
    current->component_addresses[a0].offset = a;
    current->component_addresses[a0].node = a0;

    atomic_fetch_and_add_uint64(&results, 1);    
    }
        
struct global_array *ga_allocate(
    int element_size,
    uint64  size) {
    struct global_array *ga;
    int i;
    
    ga = malloc(sizeof(*ga));
    ga->element_size = element_size;
    ga->size = size;
    ga->elements_per_node = size / gasnet_nodes();
    ga->allocated_size = ga->elements_per_node * gasnet_nodes();
    ga->component_addresses = malloc(sizeof(struct global_address) * gasnet_nodes());

    current = ga;
    results = 0;
    
    gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS);
    gasnet_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS);
    
    gm_allocate(&ga->component_addresses[gasnet_mynode()], gasnet_mynode(),
            ga->elements_per_node * ga->element_size);
    
    // communicate my location to everyone else
    for (i = 0; i < gasnet_nodes(); i++) {
        if (i != gasnet_mynode())
            gasnet_AMRequestShort3(i, GA_HANDLER, gasnet_mynode(),
                (ga->component_addresses[gasnet_mynode()].offset >> 32),
                (ga->component_addresses[gasnet_mynode()].offset & 0xffffffff));
        }
    while (results < (gasnet_nodes() - 1)) {
        address_expose();
        gasnet_AMPoll();
        }

    current = NULL;
    return ga;
    }

    