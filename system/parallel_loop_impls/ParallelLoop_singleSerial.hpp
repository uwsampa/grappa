// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.


#ifndef PARALLEL_LOOP_SINGLE_SERIAL_HPP
#define PARALLEL_LOOP_SINGLE_SERIAL_HPP

#include "SoftXMT.hpp"
#include "ForkJoin.hpp"
#include "Cache.hpp"

DECLARE_int64( parallel_loop_threshold );

/// private args will be copied to every task

namespace pl_singleSerial {

template < typename UserArg >
    struct parloop_args {
        int64_t start_index;
        int64_t iterations;
        void (*loop_body)(int64_t, UserArg *);
        GlobalAddress<Semaphore> global_sem;
        UserArg args;
    };

template < typename UserArg >
    void parallel_loop_task_wrap( parloop_args< UserArg > * args ) {
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
/// Args copied in that writes to it may or may not be reflected in
/// the original struct pointed to.
template < typename UserArg >
static void parallel_loop_helper(int64_t start_index, int64_t iterations, void (*loop_body)(int64_t, UserArg *), GlobalAddress<Semaphore> sem, UserArg args ) {
    for ( int64_t ind=0; ind<iterations; ind++ ) {
        loop_body( start_index+ind, &args );
    }
    Semaphore::release( &sem, iterations );
}

} // end namespace

static inline int64_t minint(int64_t a, int64_t b) {
    return (a<b) ? a : b;
}

template < typename UserArg >
void parallel_loop_implSingleSerial(int64_t start_index, int64_t iterations, void (*loop_body)(int64_t, UserArg *), UserArg args) {
    using namespace pl_singleSerial;

    parloop_args<UserArg> iter_args[iterations];
    Semaphore single_synch(iterations, 0);
    GlobalAddress<Semaphore> sem_addr = make_global(&single_synch);

    for (int64_t i=0; i<iterations; i+=FLAGS_parallel_loop_threshold) {
        int64_t iit = minint(FLAGS_parallel_loop_threshold, iterations-i);
        iter_args[i].start_index =  i;
        iter_args[i].iterations = iit;
        iter_args[i].loop_body = loop_body;
        iter_args[i].global_sem = sem_addr;
        iter_args[i].args = args;
        SoftXMT_publicTask( &parallel_loop_task_wrap_CachedArgs<UserArg>, make_global(&iter_args[i]) ); 
        //XXX cannot guarentee safety for very large number of iteration due to capacity; solution include make the publicQ circular (and limit number of pushes until suspend for awhile)
    }

    single_synch.acquire_all( CURRENT_THREAD );
}

#endif // PARALLEL_LOOP_SINGLE_SERIAL_HPP
