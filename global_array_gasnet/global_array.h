
#ifdef __cplusplus
extern "C" {
#endif


#ifndef _GLOBAL_ARRAY_H_
#define _GLOBAL_ARRAY_H_

#include "global_memory.h"

typedef struct global_array {
    int     element_size;
    uint64_t  size;
    uint64_t  allocated_size;
    uint64_t  elements_per_node;
    struct global_address   *component_addresses;
} global_array;

// NOTE: it is expected EVERY node in the system calls into this
// function at relatively the same time.  Failure to do so will
// lead to unpredictable results.  Internally, an anonymous barrier
// is used.
struct global_array *ga_allocate(
    int element_size,
    uint64_t size);

inline static int ga_node(struct global_array *ga, uint64_t index) {
    return index / ga->elements_per_node;
}

// Returns a global address for an element
inline static void ga_index(struct global_array *ga,
    uint64_t index, struct global_address *gm) {
      assert(index < ga->size);
//    if (!(index < ga->size)) {
//        printf("index=%lu MAX=%lu\n", index, 0L-1);
//        assert(index < ga->size);
//    }
    gm->node = index / ga->elements_per_node;
    gm->offset = ga->component_addresses[gm->node].offset +
        (index % ga->elements_per_node) * ga->element_size;
}

inline static void ga_local_range(struct global_array *ga,
    uint64_t *start, uint64_t *end) {
    *start = ga->elements_per_node * gasnet_mynode();
    *end = *start + ga->elements_per_node - 1;
    }
    
#define GA_HANDLER 131
void ga_handler(gasnet_token_t token,
    gasnet_handlerarg_t a0,
    gasnet_handlerarg_t a1,
    gasnet_handlerarg_t a2);
    

#endif // _GLOBAL_ARRAY_H_ 



#ifdef __cplusplus
}
#endif
