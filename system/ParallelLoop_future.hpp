#ifndef __PARALLEL_LOOP_FUTURE_HPP__
#define __PARALLEL_LOOP_FUTURE_HPP__

#include "SoftXMT.hpp"
#include "Future.hpp"
#include "Cache.hpp"

DECLARE_int64( parallel_loop_threshold );

/// private args will be copied to every Future

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

template < typename UserArg >
void parallel_loop_implFuture(int64_t start_index, int64_t iterations, void (*loop_body)(int64_t, UserArg *), UserArg args) {
    using namespace pl_future;
    parallel_loop_helper( start_index, iterations, loop_body, args );
}


#endif
