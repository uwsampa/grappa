
#include <stdint.h>

#include "jumpthreads.h"
#include "MCRingBuffer.h"
#include <stdio.h>
#include <assert.h>

#ifndef DEBUG
#define DEBUG 0
#endif

uint64_t delegate_jump( jthr_memdesc* memdescs, MCRingBuffer* send_queues, MCRingBuffer* receive_queues,
			uint64_t size, uint64_t procsize, uint64_t num_lists_per_thread, uint64_t num_threads ) {


  /* if (0) { */
  /*   int64_t count = size; */
    
  /*   //#include "delegate-jump.cunroll" */
  /*   while ( count > 0 ) { */
  /*     for ( int i = 0; i < num_threads; ++i ) { */
  /* 	if (DEBUG) printf( "trying to receive at count %ld\n", count ); */
  /* 	uint64_t queue_val = 0; */
  /* 	int dequeued = MCRingBuffer_consume( &send_queues[i], &queue_val ); */
  /* 	if (!dequeued) { */
  /* 	  continue; */
  /* 	} */
  /* 	if (DEBUG) printf( "receiving %p count %ld\n", (void*) queue_val, count ); */

  /* 	count--; */
  /* 	jthr_memdesc* memdesc = (jthr_memdesc*) queue_val; */
  /* 	assert( memdesc != (jthr_memdesc*) -1 ); */
  /* 	uint64_t* addr = memdesc->address; */
  /* 	if (DEBUG) printf( "got address %p count %ld\n", memdesc->address, count ); */
  /* 	uint64_t data = *addr; */
  /* 	if (DEBUG) printf( "got data %p for address %p count %ld\n", (void*) data, memdesc->address, count ); */
  /* 	memdesc->data = data; */
  /* 	memdesc->address = (uint64_t*) -1; */
  /* 	if (DEBUG) printf( "returning data %p for address %p count %ld\n", (void*) memdesc->data, memdesc->address, count ); */
  /*     } */
  /*   } */
    
  /* } */


  if (1) {
    int64_t count = size;
    
#include "delegate-jump.cunroll"
    return 0;
  }



  
  return 0;
}
