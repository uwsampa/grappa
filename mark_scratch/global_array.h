#pragma once

#include "global_memory.h"

struct global_array {
    int     element_size;
    uint64  size;
    uint64  allocated_size;
    uint64  elements_per_node;
    struct global_address   *component_addresses;
    };

// NOTE: it is expected EVERY node in the system calls into this
// function at relatively the same time.  Failure to do so will
// lead to unpredictable results.  Internally, an anonymous barrier
// is used.
struct global_array *ga_allocate(
    int element_size,
    uint64 size);

// Returns a global address for an element
inline static void ga_index(struct global_array *ga,
    uint64 index, struct global_address *gm) {
    assert(index < ga->size);
    gm->node = index / ga->elements_per_node;
    gm->offset = ga->component_addresses[gm->node].offset +
        (index % ga->elements_per_node) * ga->element_size;
}

#define GA_HANDLER 131
void ga_handler(gasnet_token_t token,
    gasnet_handlerarg_t a0,
    gasnet_handlerarg_t a1,
    gasnet_handlerarg_t a2);
    
    