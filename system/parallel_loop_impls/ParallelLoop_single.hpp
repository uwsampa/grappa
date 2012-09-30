// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.


#ifndef PARALLEL_LOOP_SINGLE_HPP
#define PARALLEL_LOOP_SINGLE_HPP

#include "SoftXMT.hpp"
#include "ForkJoin.hpp"
#include "Cache.hpp"

DECLARE_int64( parallel_loop_threshold );

/// private args will be copied to every task

namespace pl_single {

template < typename UserArg >
struct parloop_args {
    int64_t start_index;
    int64_t iterations;
    void (*loop_body)(int64_t, UserArg *);
    GlobalAddress<Semaphore> global_sem;
    UserArg args;
};

template < typename UserArg >
void parallel_loop_task_wrap( const parloop_args< UserArg > * args ) {
    parallel_loop_helper( args->start_index,
                   args->iterations,
                   args->loop_body,
                   args->global_sem,
                   args->args );// copy the loop body args
}

// TODO: for now parallel loop calls are always basic cached args style
template < typename UserArg >
DECLARE_CACHE_WRAPPED( parallel_loop_task_wrap_CachedArgs, &parallel_loop_task_wrap<UserArg>,  parloop_args<UserArg> )


/// Parallel loop with static bounds.
/// in that writes to it may or may not be reflected in
/// the original struct pointed to.
template < typename UserArg >
static void parallel_loop_helper(int64_t start_index, int64_t iterations, void (*loop_body)(int64_t, UserArg *), GlobalAddress<Semaphore> sem, GlobalAddress<parloop_args<UserArg> > args_array, UserArg args ) {

       if ( iterations == 0 ) {
           return;
       } else if ( iterations <= FLAGS_parallel_loop_threshold ) {
           for ( int64_t ind=0; ind<iterations; ind++ ) {
               loop_body( start_index+ind, &args );
           }
           Semaphore::release( &sem, iterations );
       } else {
           // spawn right half
           const parloop_args< UserArg > right_args = { start_index + (iterations+1)/2,
                            iterations/2,
                            loop_body,
                            sem,
                            args }; // copy the loop body args

           SoftXMT_publicTask( &parallel_loop_task_wrap_CachedArgs<UserArg>, make_global(&right_args) );


           // do left half
           parallel_loop_helper( start_index, 
                                 (iterations+1)/2,
                                 loop_body,
                                 sem,
                                 args ); // copy the loop body args

       }
}

} // end namespace

#include <cmath>
static inline int64_t getSizeLoopTree(int64_t iters, int64_t th) {
    int64_t depth = (int64_t) (ceil(log2(iterations)) - ceil(log2(th))); // depth of recursive split tree
    int64_t num_args_upper_bound = 1<<(depth+1); // number of calls to parallel_loop_helper
    int64_t result = num_args_upper_bound / 2; // only need to save args for calling on right side
    return result;
}

template < typename UserArg >
void parallel_loop_implSingle(int64_t start_index, int64_t iterations, void (*loop_body)(int64_t, UserArg *), UserArg args) {
    using namespace pl_single;
    
    size_t args_count = getSizeLoopTree(iterations, FLAGS_parallel_loop_threshold);
    
    GlobalAddress< parloop_args<UserArg> > args_array = SoftXMT_typed_malloc<parloop_args<UserArg> >( args_count );

    Semaphore single_synch(iterations, 0);
    parallel_loop_helper( start_index, iterations, loop_body, make_global(&single_synch), args );
    single_synch.acquire_all( CURRENT_THREAD );
}

/// Argument malloc schemes
/// need at least sizeof(parloop_args) * (floor(log2(iters)) - floor(log2(thresh)))
/// 1. malloc all on origin; everyone can index in to unique spot
//      + simple
//      + trivial deallocation
//      - hotspot origin
/// 2. SoftXMT_malloc; above but spread over Nodes
//      + same advantages as above
//      + no hotspot origin
//      - pay the penalty to get args from a different node when a task dequeues locally
/// 3. malloc at creation
//      + keep args local if dequeued locally
//      + average memory usage may be lower since memory is freed as we go up the tree, but worst there will be a point when peak usage is same: everything is malloc'd (get to leaves)
//      - dynamic allocation in critical path
//      - more complicated: need to pass pointer to task to free the args (could encapsulate this in a wrapper though)
/// 4. #2 + #3 hybrid: allocate the space beforehand and use simple bump allocate on a node
//      + less allocation overhead than #3
//      + keep memory local if dequeued locally
//      - need to allocate extra space pessimistically because of load imbalance (could do optimistic allocation size and then just double size on overflow)


#endif // PARALLEL_LOOP_SINGLE_HPP
