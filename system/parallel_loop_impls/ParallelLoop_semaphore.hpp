#ifndef __PARALLEL_LOOP_SEMAPHORE_HPP__
#define __PARALLEL_LOOP_SEMAPHORE_HPP__

#include "SoftXMT.hpp"
#include "ForkJoin.hpp"
#include "Cache.hpp"

DECLARE_int64( parallel_loop_threshold );

/// private args will be copied to every Future

namespace pl_semaphore {

template < typename UserArg >
struct parloop_args {
    int64_t start_index;
    int64_t iterations;
    void (*loop_body)(int64_t, UserArg *);
    GlobalAddress<Semaphore> sem;
    UserArg args;
};

template < typename UserArg >
void parallel_loop_task_wrap( const parloop_args< UserArg > * args ) {
    parallel_loop_helper( args->start_index,
                   args->iterations,
                   args->loop_body,
                   args->sem,
                   args->args ); // copy the loop body args
}

// TODO: for now parallel loop calls are always basic cached args style
template < typename UserArg >
DECLARE_CACHE_WRAPPED( parallel_loop_task_wrap_CachedArgs, &parallel_loop_task_wrap<UserArg>, parloop_args<UserArg> )


/// Parallel loop with static bounds.
/// in that writes to it may or may not be reflected in
/// the original struct pointed to.
template < typename UserArg >
static void parallel_loop_helper(int64_t start_index, int64_t iterations, void (*loop_body)(int64_t, UserArg *), GlobalAddress<Semaphore> parent_sem, UserArg args) {
       if ( iterations == 0 ) {
           return;
       } else if ( iterations <= FLAGS_parallel_loop_threshold ) {
           for ( int64_t ind=0; ind<iterations; ind++ ) {
               loop_body( start_index+ind, &args );
           }
       } else {
           Semaphore s(2, 0);
           GlobalAddress<Semaphore> gs = make_global( &s );
           
           // spawn right half
           parloop_args< UserArg > right_args = { start_index + (iterations+1)/2,
                            iterations/2,
                            loop_body,
                            gs,
                            args
                           }; // copy the loop body args
           SoftXMT_publicTask( SoftXMT_magic_identity_function(&parallel_loop_task_wrap_CachedArgs<UserArg>), make_global( &right_args ));
           
           // spawn left half
           parloop_args< UserArg > left_args = { start_index,
                            (iterations+1)/2,
                            loop_body,
                            gs,
                            args
                           }; // copy the loop body args
           SoftXMT_publicTask( SoftXMT_magic_identity_function(&parallel_loop_task_wrap_CachedArgs<UserArg>), make_global( &left_args ));
           
           VLOG(3) << "doing left: " << left_args.start_index << " iters " << left_args.iterations <<
                        "\n  and right: " << right_args.start_index << " iters " << right_args.iterations;

           // join on the halves
           s.acquire_all( CURRENT_THREAD );
       }
       // notify parent 
       Semaphore::release( &parent_sem, 1 );
}

} // end namespace

template < typename UserArg >
void parallel_loop_implSemaphore(int64_t start_index, int64_t iterations, void (*loop_body)(int64_t, UserArg *), UserArg args) {
    using namespace pl_semaphore;
       
    Semaphore s( 1, 0 ); // don't need this, just to pass something valid to the first parallel_loop_helper call
    GlobalAddress<Semaphore> gs = make_global( &s );
    
    VLOG(2) << "starting semaphore parallel loop";
    parallel_loop_helper( start_index, iterations, loop_body, gs, args );

    s.acquire_all( CURRENT_THREAD );   // unneeded because first call joins already
}


#endif
