// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef PARALLEL_LOOP_FUTURE_HPP
#define PARALLEL_LOOP_FUTURE_HPP

#include "Grappa.hpp"
#include "Future.hpp"
#include "Cache.hpp"

DECLARE_int64( parallel_loop_threshold );

/// Private args will be copied to every Future

namespace pl_future {

template < typename UserArg >
struct parloop_args {
    int64_t start_index;
    int64_t iterations;
    void (*loop_body)(int64_t, UserArg *);
    UserArg args;
};

template < typename UserArg >
void parallel_loop_task_wrap( const parloop_args< UserArg > * args ) {
    parallel_loop_helper( args->start_index,
                   args->iterations,
                   args->loop_body,
                   args->args ); // copy the loop body args
}

// TODO: for now parallel loop calls are always basic cached args style
template < typename UserArg >
DECLARE_CACHE_WRAPPED( parallel_loop_task_wrap_CachedArgs, &parallel_loop_task_wrap<UserArg>,  parloop_args<UserArg> )


/// Parallel loop with static bounds.
/// in that writes to it may or may not be reflected in
/// the original struct pointed to.
template < typename UserArg >
static void parallel_loop_helper(int64_t start_index, int64_t iterations, void (*loop_body)(int64_t, UserArg *), UserArg args) {
       if ( iterations == 0 ) {
           return;
       } else if ( iterations <= FLAGS_parallel_loop_threshold ) {
           for ( int64_t ind=0; ind<iterations; ind++ ) {
               loop_body( start_index+ind, &args );
           }
       } else {
           // spawn right half
           const parloop_args< UserArg > right_args = { start_index + (iterations+1)/2,
                            iterations/2,
                            loop_body,
                            args
                           }; // copy the loop body args

           Future< GlobalAddress<parloop_args<UserArg> >, &parallel_loop_task_wrap_CachedArgs<UserArg> >  d ( make_global( &right_args ) );
           d.addAsPublicTask( );
            
           // do left half
           parallel_loop_helper( start_index, (iterations+1)/2,
                          loop_body,
                          args); // copy the loop body args

           // touch
           d.touch( );
       }
}

} // end namespace

/// Parallel loop with indices start_index to iterations. Iterations may
/// execute in any order, and this call will block until all iterations are completed.
/// This parallel loop is implemented with recursive decomposition ala Cilk_for, and it
/// uses Future for the join at each split. (currently touch will occur when the iterations are still local)
///
/// @tparam UserArg type of the user args
/// @param start_index starting index
/// @param iterations number of iterations
/// @param loop_body function pointer representing the loop body for iteration i, also gets copy of user arguments
///
/// Note: currently this parallel loop implementation is known to be less performant than the
///       AsyncParallelFor + GlobalTaskJoiner used in current benchmarks
template < typename UserArg >
void parallel_loop_implFuture(int64_t start_index, int64_t iterations, void (*loop_body)(int64_t, UserArg *), UserArg args) {
    using namespace pl_future;
    parallel_loop_helper( start_index, iterations, loop_body, args );
}


#endif // PARALLEL_LOOP_FUTURE_HPP
