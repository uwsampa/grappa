
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#include <omp.h>

#define GASNET_CONDUIT_IBV 1
#include <gasnet.h>
#include <gasnet_bootstrap_internal.h>

#include <sys/mman.h>

#include <psm.h>
#include <psm_am.h>
#include "psm_utils.h"

#include "node.h"
#include "alloc.h"
#include "debug.h"

#include "options.h"

#include "thread.h"


#define DEBUG 0

static gasnet_node_t mynode = -1;
static gasnet_node_t num_nodes = 0;



typedef struct stack_element {
  uint64_t d1;
  uint64_t d2;
} stack_element_t;

#define STACK_DATA_T stack_element_t
#include "stack.h"

//#include "jumpthreads.h"
//#include "MCRingBuffer.h"
//#include "walk.h"

stack_t s;

inline int islocal( void * addr ) {
  return 0;
}

inline int mycounterpart() {
  if (mynode < num_nodes / 2) {
    return mynode + num_nodes / 2;
  } else {
    return mynode - num_nodes / 2;
  }
}

inline int addr2node( void * addr ) {
  //return 1 - mynode;
  return mycounterpart();
}

int network_done = 0;
int local_done = 0;
int local_active = 0;

int AM_NODE_REQUEST_HANDLER = -1;
int am_node_request(psm_am_token_t token, psm_epaddr_t epaddr,
	  psm_amarg_t *args, int nargs, 
	  void *src, uint32_t len) {
  stack_element_t sel;
  sel.d1 = args[0].u64;
  sel.d2 = args[1].u64;
  if (DEBUG) printf("Node %d got request for %p for %p\n", mynode, sel.d1, sel.d2);
  stack_push( &s, sel );
  return 0;
}

int AM_NODE_REPLY_HANDLER = -1;
int am_node_reply(psm_am_token_t token, psm_epaddr_t epaddr,
	  psm_amarg_t *args, int nargs, 
	  void *src, uint32_t len) {
  uint64_t * dest_addr = (uint64_t *) args[0].u64;
  uint64_t data = args[1].u64;
  uint64_t * dest_addr2 = (uint64_t *) args[2].u64;
  uint64_t data2 = args[3].u64;
  if (DEBUG) printf("Node %d got response of %p for %p\n", mynode, data, dest_addr);
  *dest_addr = data;
  if( nargs == 4 ) *dest_addr2 = data2;
  return 0;
}

int AM_QUIT_HANDLER = -1;
int am_quit(psm_am_token_t token, psm_epaddr_t epaddr,
	    psm_amarg_t *args, int nargs, 
	    void *src, uint32_t len) {
  //network_done -= args[0].u64;
  if (DEBUG) printf("Node %d got quit request\n", mynode);
  network_done = 1;
  return 0;
}

void setup_ams( psm_ep_t ep ) {
  psm_am_handler_fn_t handlers[] = { &am_node_request, &am_node_reply, &am_quit };
  int handler_indices[3] = { 0 };
  PSM_CHECK( psm_am_register_handlers( ep, handlers, 3, handler_indices ) );
  printf("AM_NODE_REQUEST_HANDLER = %d\n", (AM_NODE_REQUEST_HANDLER = handler_indices[0]) );
  printf("AM_NODE_REPLY_HANDLER = %d\n", (AM_NODE_REPLY_HANDLER = handler_indices[1]) );
  printf("AM_QUIT_HANDLER = %d\n", (AM_QUIT_HANDLER = handler_indices[2]) );
  PSM_CHECK( psm_am_activate(ep) );
}



void request( void * addr, void * dest_addr ) {
  psm_amarg_t args[2];
  args[0].u64 = (uint64_t) addr;
  args[1].u64 = (uint64_t) dest_addr;
  if (DEBUG) printf("Node %d sending request for %p for %p\n", mynode, addr, dest_addr);
  PSM_CHECK( psm_am_request_short( dest2epaddr( addr2node( addr ) ), AM_NODE_REQUEST_HANDLER, 
				   args, 2, NULL, 0,
				   PSM_AM_FLAG_ASYNC | PSM_AM_FLAG_NOREPLY,
				   NULL, NULL ) );
}

void reply( uint64_t dest_addr, uint64_t data ) {
  psm_amarg_t args[2];
  args[0].u64 = dest_addr;
  args[1].u64 = data;
  if (DEBUG) printf("Node %d sending reply for %p for %p\n", mynode, dest_addr, data);
  PSM_CHECK( psm_am_request_short( dest2epaddr( addr2node( (void*) dest_addr ) ), AM_NODE_REPLY_HANDLER, 
				   args, 2, NULL, 0,
				   PSM_AM_FLAG_ASYNC | PSM_AM_FLAG_NOREPLY,
				   NULL, NULL ) );
}


void request2( void * addr, void * dest_addr ) {
  static psm_amarg_t args[4];
  static int off = 0;
  args[0+off].u64 = (uint64_t) addr;
  args[1+off].u64 = (uint64_t) dest_addr;
  if (DEBUG) printf("Node %d sending request for %p for %p\n", mynode, addr, dest_addr);
  if (off == 2) {
    PSM_CHECK( psm_am_request_short( dest2epaddr( addr2node( addr ) ), AM_NODE_REQUEST_HANDLER, 
				     args, 4, NULL, 0,
				     PSM_AM_FLAG_ASYNC | PSM_AM_FLAG_NOREPLY,
				     NULL, NULL ) );
    off = 0;
  } else { off += 2; }
}

void reply2( uint64_t dest_addr, uint64_t data,  uint64_t dest_addr2, uint64_t data2 ) {
  psm_amarg_t args[4];
  args[0].u64 = dest_addr;
  args[1].u64 = data;
  args[2].u64 = dest_addr2;
  args[3].u64 = data2;
  if (DEBUG) printf("Node %d sending reply for %p for %p\n", mynode, dest_addr, data);
  PSM_CHECK( psm_am_request_short( dest2epaddr( addr2node( (void*) dest_addr ) ), AM_NODE_REPLY_HANDLER, 
				   args, 4, NULL, 0,
				   PSM_AM_FLAG_ASYNC | PSM_AM_FLAG_NOREPLY,
				   NULL, NULL ) );
}

void quit() {
  if (DEBUG) printf("Node %d sending quit request\n", mynode);
  PSM_CHECK( psm_am_request_short( dest2epaddr( mycounterpart() ), AM_QUIT_HANDLER, 
				   NULL, 0, NULL, 0,
				   PSM_AM_FLAG_ASYNC | PSM_AM_FLAG_NOREPLY,
				   NULL, NULL ) );
}


struct mailbox {
  uint64_t data;
  int full;
};

inline void issue( struct node ** pointer, struct mailbox * mb ) {
  if( islocal( pointer ) ) {
    __builtin_prefetch( pointer, 0, 0 );
  } else {
    mb->data = 0;
    request( pointer, &mb->data );
  }
}

inline struct node * complete( thread * me, struct node ** pointer, struct mailbox * mb ) {
  if( islocal( pointer ) ) {
    return *pointer;
  } else {
    while( 0 == mb->data ) {
      thread_yield( me );
    }
    return (struct node *) mb->data;
  }
}

uint64_t walk_prefetch_switch( thread * me, node * current, uint64_t count ) {
  struct mailbox mb;
  mb.data = 0;
  mb.full = 0;

  while( count > 0 ) {
    --count;
    issue( &current->next, &mb );
    thread_yield( me );
    current = complete( me, &current->next, &mb );
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
  if (DEBUG) printf("Node %d thread starting\n", mynode); fflush(stdout);
  args->result = walk_prefetch_switch( me, args->base, args->count );
  --local_active;
  if( local_active == 0 ) {
    local_done = 1;
    if (DEBUG) printf("Node %d Calling quit\n", mynode); fflush(stdout);
    quit();
    if (DEBUG) printf("Node %d Quit called\n", mynode); fflush(stdout);
  }
  if (DEBUG) printf("returned %lx\n", args->result);
}


struct network_args {
  stack_t * s;
};

void network_runnable( thread * me, void * argsv ) {
  struct network_args * args = argsv;
  psm_ep_t ep = get_ep();
  if (DEBUG) printf("Node %d network thread starting\n", mynode); fflush(stdout);
  while( network_done == 0 || local_done == 0 ) {
    //printf("node %d network_done = %d local_done = %d\n", mynode, network_done, local_done );
    psm_poll(ep);

    if (0) {
      if( stack_check( args->s ) ) {
	stack_element_t sel = stack_get( args->s );
	uint64_t * addr = (uint64_t *) sel.d1;
	stack_pop( args->s );
	uint64_t * addr2 = addr;
	stack_element_t sel2 = sel;
	
	/* if( stack_check( args->s ) ) { */
	/* 	sel2 = stack_get( args->s ); */
	/* 	addr2 = (uint64_t *) sel2.d1; */
	/* 	stack_pop( args->s ); */
	/* } */
	
	__builtin_prefetch( addr, 0, 0 );
	/* __builtin_prefetch( addr2, 0, 0 ); */
	
	thread_yield( me );
	
	//reply2( sel.d2, *addr, sel2.d2, *addr2 );
	reply( sel.d2, *addr );
      } else {
	thread_yield( me );
      }
    }

    if (1) {
      if( stack_check( args->s ) ) {
	while( stack_check( args->s ) ) {
	  stack_element_t sel = stack_get( args->s );
	  uint64_t * addr = (uint64_t *) sel.d1;
	  stack_pop( args->s );
	  
	  __builtin_prefetch( addr, 0, 0 );
	  
	  thread_yield( me );
	  
	  reply( sel.d2, *addr );
	}
      } else {
	thread_yield( me );
      }
    }


  }
  if (DEBUG) printf("Node %d network thread done\n", mynode); fflush(stdout);
}




int main( int argc, char * argv[] ) {

  //ASSERT_Z( gasnet_init(&argc, &argv) );

  gasnet_node_t gasneti_mynode = -1;
  gasnet_node_t gasneti_nodes = 0;

  gasneti_bootstrapInit_ssh( &argc, &argv, &gasneti_nodes, &gasneti_mynode );
  printf("initialized node %d of %d on %s\n", gasneti_mynode, gasneti_nodes, gasneti_gethostname() );

  mynode = gasneti_mynode;
  num_nodes = gasneti_nodes;
  
  psm_setup( gasneti_mynode, gasneti_nodes);
  setup_ams( get_ep() );
  printf("Node %d PSM is set up.\n", mynode); fflush(stdout);

  //ASSERT_Z( gasnet_attach( NULL, 0, 0, 0 ) );
  //printf("hello from node %d of %d\n", gasnet_mynode(), gasnet_nodes() );

  struct options opt_actual = parse_options( &argc, &argv );
  struct options * opt = &opt_actual;

  printf("Node %d allocating heap.\n", mynode); fflush(stdout);
  node * nodes = NULL;
  node ** bases = NULL;
  void* nodes_start = (void*) 0x0000111122220000UL;
  nodes = mmap( nodes_start, sizeof(node) * opt->list_size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, 0, 0 );
  assert( nodes == nodes_start );

  allocate_heap(opt->list_size, opt->cores, opt->threads, &bases, &nodes);
  printf("Node %d base address is %p.\n", mynode, nodes); fflush(stdout);

  int i, core_num;

  thread * threads[ opt->cores ][ opt->threads ];
  thread * masters[ opt->cores ];
  scheduler * schedulers[ opt->cores ];

  struct thread_args args[ opt->cores ][ opt->threads ];

  struct network_args netargs;
  stack_init( &s, 1024 );
  netargs.s = &s;
  printf("Node %d spawning threads.\n", mynode); fflush(stdout);
#pragma omp parallel for num_threads( opt->cores )
  for( core_num = 0; core_num < opt->cores; ++core_num ) {
    masters[ core_num ] = thread_init();
    schedulers[ core_num ] = create_scheduler( masters[ core_num ] );

    int thread_num;
    for( thread_num = 0; thread_num < opt->threads; ++thread_num ) {
      args[ core_num ][ thread_num ].base = bases[ core_num * opt->threads + thread_num ];
      args[ core_num ][ thread_num ].count = opt->list_size * opt->count / (opt->cores * opt->threads);
      args[ core_num ][ thread_num ].result = 0;
      if (1) {
	if (mynode < num_nodes / 2) {
	  threads[ core_num ][ thread_num ] = thread_spawn( masters[ core_num ], schedulers[ core_num ], 
							    thread_runnable, &args[ core_num ][ thread_num ] );
	  ++local_active;
	  network_done = 1;
	} else { local_done = 1; }
      }
      if (0) {
	threads[ core_num ][ thread_num ] = thread_spawn( masters[ core_num ], schedulers[ core_num ], 
							  thread_runnable, &args[ core_num ][ thread_num ] );
	++local_active;
      }
      thread_spawn( masters[ core_num ], schedulers[ core_num ], network_runnable, &netargs );
    }
  }

  printf("Node %d starting.\n", mynode); fflush(stdout);

  struct timespec start_time;
  struct timespec end_time;

  gasneti_bootstrapBarrier_ssh();
  clock_gettime(CLOCK_MONOTONIC, &start_time);
#pragma omp parallel for num_threads( opt->cores )
  for( core_num = 0; core_num < opt->cores; ++core_num ) {
    run_all( schedulers[ core_num ] );
  }
  clock_gettime(CLOCK_MONOTONIC, &end_time);
  gasneti_bootstrapBarrier_ssh();

  printf("Node %d done.\n", mynode); fflush(stdout);

  uint64_t sum;
#pragma omp parallel for num_threads( opt->cores ) reduction( + : sum )
  for( core_num = 0; core_num < opt->cores; ++core_num ) {
    int thread_num;
    for( thread_num = 0; thread_num < opt->threads; ++thread_num ) {
      sum += args[ core_num ][ thread_num ].result;
    }
  }

  double runtime = (end_time.tv_sec + 1.0e-9 * end_time.tv_nsec) - (start_time.tv_sec + 1.0e-9 * start_time.tv_nsec);
  double rate = (double) opt->list_size * opt->count * (num_nodes / 2) / runtime;
  double bandwidth = (double) opt->list_size * opt->count * sizeof(node) * (num_nodes / 2) / runtime;

  //  if( 0 == gasnet_mynode() ) {
    printf("header, list_size, count, processes, threads, runtime, message rate (M/s), bandwidth (MB/s), sum\n");
    printf("data, %d, %d, %d, %d, %f, %f, %f, %d\n", opt->list_size, opt->count, 1, opt->threads, runtime, rate / 1000000, bandwidth / (1024 * 1024), sum);
    //  }

  psm_finalize();
  gasneti_bootstrapFini_ssh();
  printf("exiting on node %d on %s\n", gasneti_mynode, gasneti_gethostname() );
  //gasnet_exit(0);
  return 0;
}
