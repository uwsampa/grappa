#include <gasnet.h>
#include <assert.h>
#include "collective.h"

// Active messages implementation of reductions.
// Only one reduction can happen at a time.

// GCC atomic builtin
#define atomic_fetch_and_add(PTR, VALUE) __sync_fetch_and_add((PTR), (VALUE))


#define _MAX_PROC_NODES 16
// only the home_node uses it's version of these two local arrays
/*volatile*/ uint64_t values[_MAX_PROC_NODES]; 
/*volatile*/ int doneCount;

gasnet_hsl_t lock = GASNET_HSL_INITIALIZER;

volatile uint64_t ringValue;
volatile int doneRing;

void serialReduceRequestHandler(gasnet_token_t token, gasnet_handlerarg_t a0) {
    uint64_t reduceVal = (uint64_t) a0;

    // find who it's from to know where to put it
    gasnet_node_t fromNode;
    assert(GASNET_OK == gasnet_AMGetMsgSource(token, &fromNode));

    int val;
    gasnet_hsl_lock(&lock);
    values[fromNode] = reduceVal;
    //asm volatile ("" ::: "memory");
    //uint64_t val = atomic_fetch_and_add(&doneCount, 1);
    val = doneCount++;
    gasnet_hsl_unlock(&lock);

    printf("received from %d cause done count %d\n", fromNode, val+1);
}



uint64_t serialReduce(uint64_t (*commutative_func)(uint64_t, uint64_t), uint64_t home_node, uint64_t myValue ) {
    if (gasnet_mynode()==home_node) {
        const uint64_t num_nodes = gasnet_nodes();
        doneCount = 0;
        values[home_node] = myValue;

        //GASNET_BLOCKUNTIL (doneCount == num_nodes-1);
        
        int can_break = 0;
        while (!can_break) {
            gasnet_AMPoll();
            gasnet_hsl_lock(&lock);
            if (doneCount==num_nodes-1) {
                can_break = 1;
            }
            gasnet_hsl_unlock(&lock);
        }


        // perform the reduction
        uint64_t sofar = values[0];
        gasnet_hsl_lock(&lock);
        for (int i=1; i<num_nodes; i++) {
           sofar = (*commutative_func)(sofar, values[i]);
        }
        gasnet_hsl_unlock(&lock);

        return sofar;
    } else {
        gasnet_AMRequestShort1(home_node, COLLECTIVE_REDUCTION_HANDLER, myValue);

        /* nothing left to do; reduction goes on home */
        return 0;
    }
}

uint64_t ringReduce(uint64_t (*commutative_func)(uint64_t, uint64_t), uint64_t home_node, uint64_t myValue ) {
    int rank = gasnet_mynode();
    int num_nodes = gasnet_nodes();
    uint64_t result;
    doneRing = 0;
    
    gasnet_barrier_notify(22, 0);
    gasnet_barrier_wait(22, 0);
    
    if (rank == home_node) {
        gasnet_AMRequestShort1((rank+1)%num_nodes, COLLECTIVE_RING_REDUCTION_HANDLER, myValue);
        GASNET_BLOCKUNTIL (doneRing);
        result = ringValue;
    } else {
        GASNET_BLOCKUNTIL (doneRing);
        result = (*commutative_func)(ringValue, myValue);
        gasnet_AMRequestShort1((rank+1)%num_nodes, COLLECTIVE_RING_REDUCTION_HANDLER, result);
    }
    return result;
}

void ringReduceRequestHandler(gasnet_token_t token, gasnet_handlerarg_t a0) {
    uint64_t sofar = (uint64_t) a0;
    ringValue = sofar;
    asm volatile ("" ::: "memory");
    doneRing = 1;
}
    



uint64_t collective_max(uint64_t a, uint64_t b) { return (a>b) ? a : b; }
uint64_t collective_min(uint64_t a, uint64_t b) { return (a<b) ? a : b; }
uint64_t collective_add(uint64_t a, uint64_t b) { return a+b; }
uint64_t collective_mult(uint64_t a, uint64_t b) { return a*b; }
