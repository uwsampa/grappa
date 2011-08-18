#ifndef _COLLECTIVE_H_
#define _COLLECTIVE_H_

#include <stdint.h>

uint64_t collective_max(uint64_t a, uint64_t b);
uint64_t collective_min(uint64_t a, uint64_t b);
uint64_t collective_add(uint64_t a, uint64_t b);
uint64_t collective_mult(uint64_t a, uint64_t b);

#define COLL_MAX &collective_max
#define COLL_MIN &collective_min
#define COLL_ADD &collective_add
#define COLL_MULT &collective_mult


// Reduction is done serially. Only the home_node will return the correct result; other returns undefined. Reductions with same home_node cannot be done concurrently.
uint64_t serialReduceLong(uint64_t (*commutative_func)(uint64_t, uint64_t), uint64_t home_node, uint64_t myValue );


// don't want these impl specific things in header
#include <gasnet.h>
#define COLLECTIVE_REDUCTION_HANDLER 211
void serialReduceRequestHandler(gasnet_token_t token, gasnet_handlerarg_t a0);


#endif // _COLLECTIVE_H_
