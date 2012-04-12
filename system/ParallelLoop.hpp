#ifndef __PARALLEL_LOOP_HPP__
#define __PARALLEL_LOOP_HPP__

#include "SoftXMT.hpp"
#include "Future.hpp"

static int64_t threshold = 1;

template < typename ArgsStruct >
struct parloop_args {
    int64_t start_index;
    int64_t iterations;
    void (*loop_body)(int64_t i, ArgsStruct * sargs);
    ArgsStruct * shared_args;
};

template < typename ArgsStruct >
static void parallel_loop_task_wrap( parloop_args< ArgsStruct > * args ) {
    parallel_loop( args->start_index,
                   args->iterations,
                   args->loop_body,
                   args->shared_args );
}

/// Parallel loop with static bounds.
/// Shared_args should be treated as RO,
/// in that writes to it may or may not be reflected in
/// the original struct pointed to.
template < typename ArgsStruct >
void parallel_loop(int64_t start_index, int64_t iterations, void (*loop_body)(int64_t i, ArgsStruct * sargs), ArgsStruct * shared_args) {
       if ( iterations == 0 ) {
           return;
       } else if ( iterations <= threshold ) {
           for ( int64_t ind=0; ind<iterations; ind++ ) {
               loop_body( start_index+ind, shared_args );
           }
       } else {
           // spawn right half
           parloop_args< ArgsStruct > right_args = { start_index + (iterations+1)/2,
                            iterations/2,
                            loop_body,
                            shared_args };

           Future< parloop_args<ArgsStruct> >  d ( &parallel_loop_task_wrap, &right_args );
           d.asPublicTask( );
            
           // do left half
           parallel_loop( start_index, (iterations+1)/2,
                          loop_body,
                          shared_args);

           // touch
           d.touch( );
       }
}

#endif
