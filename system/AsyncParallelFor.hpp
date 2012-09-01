#ifndef __ASYNC_PARALLEL_FOR__
#define __ASYNC_PARALLEL_FOR__

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <stdint.h>

// If the default threshold value is used, then use the command line flag value
#define ASYNC_PAR_FOR_DEFAULT -1
DECLARE_int64( async_par_for_threshold );

template < void (*LoopBody)(int64_t,int64_t),
           void (*Spawn)(int64_t,int64_t),
           int64_t Threshold >
void async_parallel_for( int64_t start, int64_t iterations ) {
  DVLOG(5) << "[" << start << "," << start+iterations << ")";
  if ( iterations == 0 ) { // TODO: remove this redundant branch
    return;
  } else if ( Threshold == ASYNC_PAR_FOR_DEFAULT ) { // template specialization that should compile out
    if ( iterations <= FLAGS_async_par_for_threshold ) {
      LoopBody ( start, iterations );
      return;
    }
  } else if ( iterations <= Threshold ) {
    LoopBody( start, iterations );
    return;
  }

  // asyncronous spawn right half
  Spawn ( start + (iterations+1)/2, iterations/2 );

  // go down left half
  async_parallel_for<LoopBody,Spawn,Threshold>( start, (iterations+1)/2 );
}



#include "Addressing.hpp"

template < typename Arg,
           void (*LoopBody)(int64_t,int64_t,Arg),
           void (*Spawn)(int64_t,int64_t,Arg),
           int64_t Threshold >
void async_parallel_for( int64_t start, int64_t iterations, Arg shared_arg ) {
  DVLOG(5) << "[" << start << "," << start+iterations << ")";
  if ( iterations == 0 ) { // TODO: remove this redundant branch
    return;
  } else if ( Threshold == ASYNC_PAR_FOR_DEFAULT ) {    // template specialization that should compile out
    if ( iterations <= FLAGS_async_par_for_threshold ) {
      LoopBody ( start, iterations, shared_arg );
      return;
    }
  } else if ( iterations <= Threshold ) {
    LoopBody( start, iterations, shared_arg );
    return;
  }

  // asyncronous spawn right half
  Spawn ( start + (iterations+1)/2, iterations/2, shared_arg );

  // go down left half
  async_parallel_for<Arg,LoopBody,Spawn,Threshold>( start, (iterations+1)/2, shared_arg );
}

// redeclaration to handle special GlobalAddress case used in some places
template < typename Arg,
           void (*LoopBody)(int64_t,int64_t,GlobalAddress<Arg>),
           void (*Spawn)(int64_t,int64_t,GlobalAddress<Arg>),
           int64_t Threshold >
void async_parallel_for( int64_t start, int64_t iterations, GlobalAddress<Arg> shared_arg ) {
  async_parallel_for<GlobalAddress<Arg>,LoopBody,Spawn,Threshold>(start, iterations, shared_arg);
}

#endif // __ASYNC_PARALLEL_FOR__
