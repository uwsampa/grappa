#ifndef __ASYNC_PARALLEL_FOR__
#define __ASYNC_PARALLEL_FOR__

#include <gflags/gflags.h>
#include <stdint.h>

DECLARE_int64( async_par_for_threshold );


template < void (*L)(int64_t,int64_t), 
           void (*S)(void (*)(int64_t,int64_t),int64_t,int64_t) >
void async_parallel_for( int64_t start, int64_t iterations ) {
  if ( iterations == 0 ) {
    return;
  } else if ( iterations <= FLAGS_async_par_for_threshold ) {
    L ( start, iterations );
  } else {
    // asyncronous spawn right half
    S ( &async_parallel_for<L,S>, start + (iterations+1)/2, iterations/2 );
   
    // go down left half
    async_parallel_for<L,S>( start, (iterations+1)/2 );
  }
}

#endif // __ASYNC_PARALLEL_FOR__
