
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#include <omp.h>


#include <gasnet.h>

#include "node.h"
#include "alloc.h"
#include "debug.h"

#include "options.h"

#include "thread.h"

//#include "jumpthreads.h"
//#include "MCRingBuffer.h"
//#include "walk.h"

void issue( struct node ** pointer ) {
  __builtin_prefetch( pointer, 0, 0 );
}

struct node * complete( struct node ** pointer ) {
  return *pointer;
}

uint64_t walk_prefetch_switch( thread * me, node * current, uint64_t count ) {
  while( count > 0 ) {
    --count;
    issue( &current->next );
    thread_yield( me );
    current = complete( &current->next );
  }
  return (uint64_t) current;
}

struct thread_args {
  node * base;
  uint64_t count;
  uint64_t result;
};

void thread_runnable( thread * me, void * argsv ) {
  struct thread_args * args = argsv;
  args->result = walk_prefetch_switch( me, args->base, args->count );
  printf("returned %lx\n", args->result);
}


int main( int argc, char * argv[] ) {

  ASSERT_Z( gasnet_init(&argc, &argv) );
  ASSERT_Z( gasnet_attach( NULL, 0, 0, 0 ) );
  printf("hello from node %d of %d\n", gasnet_mynode(), gasnet_nodes() );

  struct options opt_actual = parse_options( &argc, &argv );
  struct options * opt = &opt_actual;

  node * nodes = NULL;
  node ** bases = NULL;
  allocate_heap(opt->list_size, opt->cores, opt->threads, &bases, &nodes);
    
  int i, core_num;

  thread * threads[ opt->cores ][ opt->threads ];
  thread * masters[ opt->cores ];
  scheduler * schedulers[ opt->cores ];

  struct thread_args args[ opt->cores ][ opt->threads ];

#pragma omp parallel for num_threads( opt->cores )
  for( core_num = 0; core_num < opt->cores; ++core_num ) {
    masters[ core_num ] = thread_init();
    schedulers[ core_num ] = create_scheduler( masters[ core_num ] );

    int thread_num;
    for( thread_num = 0; thread_num < opt->threads; ++thread_num ) {
      args[ core_num ][ thread_num ].base = bases[ core_num * opt->threads + thread_num ];
      args[ core_num ][ thread_num ].count = opt->list_size * opt->count / (opt->cores * opt->threads);
      args[ core_num ][ thread_num ].result = 0;
      threads[ core_num ][ thread_num ] = thread_spawn( masters[ core_num ], schedulers[ core_num ], thread_runnable, 
							&args[ core_num ][ thread_num ] );
    }
  }

  struct timespec start_time;
  struct timespec end_time;

  clock_gettime(CLOCK_MONOTONIC, &start_time);
#pragma omp parallel for num_threads( opt->cores )
  for( core_num = 0; core_num < opt->cores; ++core_num ) {
    run_all( schedulers[ core_num ] );
  }
  clock_gettime(CLOCK_MONOTONIC, &end_time);

  uint64_t sum;
#pragma omp parallel for num_threads( opt->cores ) reduction( + : sum )
  for( core_num = 0; core_num < opt->cores; ++core_num ) {
    int thread_num;
    for( thread_num = 0; thread_num < opt->threads; ++thread_num ) {
      sum += args[ core_num ][ thread_num ].result;
    }
  }

  double runtime = (end_time.tv_sec + 1.0e-9 * end_time.tv_nsec) - (start_time.tv_sec + 1.0e-9 * start_time.tv_nsec);
  double rate = (double) opt->list_size * opt->count / runtime;
  double bandwidth = (double) opt->list_size * opt->count * sizeof(node) / runtime;

  //  if( 0 == gasnet_mynode() ) {
    printf("header, list_size, count, processes, threads, runtime, message rate (M/s), bandwidth (MB/s), sum\n");
    printf("data, %d, %d, %d, %d, %f, %f, %f, %d\n", opt->list_size, opt->count, 1, opt->threads, runtime, rate / 1000000, bandwidth / (1024 * 1024), sum);
    //  }


    gasnet_exit(0);
  return 0;
}
