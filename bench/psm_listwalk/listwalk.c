
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

#include <MCRingBuffer.h>

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
MCRingBuffer q;

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
int request_received = 0;
int reply_received = 0;

int AM_NODE_REQUEST_HANDLER = -1;
int am_node_request(psm_am_token_t token, psm_epaddr_t epaddr,
	  psm_amarg_t *args, int nargs, 
	  void *src, uint32_t len) {
  exit(1);
  stack_element_t sel;
  sel.d1 = args[0].u64;
  sel.d2 = args[1].u64;
  if (DEBUG) { printf("Node %d got request for %p for %p\n", mynode, sel.d1, sel.d2); }
  stack_push( &s, sel );
  ++request_received;
  return 0;
}

int AM_NODE_REPLY_HANDLER = -1;
int am_node_reply(psm_am_token_t token, psm_epaddr_t epaddr,
	  psm_amarg_t *args, int nargs, 
	  void *src, uint32_t len) {
  exit(1);
  uint64_t * dest_addr = (uint64_t *) args[0].u64;
  uint64_t data = args[1].u64;
  if (DEBUG) { printf("Node %d got response of %p for %p\n", mynode, data, dest_addr); }
  *dest_addr = data;
  ++reply_received;
  return 0;
}

int AM_NODE_REQUEST_AGG_HANDLER = -1;
int am_node_request_agg(psm_am_token_t token, psm_epaddr_t epaddr,
			psm_amarg_t *args, int nargs, 
			void *src, uint32_t len) {
  exit(1);
  stack_element_t sel;
  int off;
  for( off = 0; off < nargs; off += 2 ) {
    sel.d1 = args[off+0].u64;
    sel.d2 = args[off+1].u64;
    if (DEBUG) { printf("Node %d got request for %p for %p\n", mynode, sel.d1, sel.d2); }
    stack_push( &s, sel );
    ++request_received;
  }
  return 0;
}

int AM_NODE_REPLY_AGG_HANDLER = -1;
int am_node_reply_agg(psm_am_token_t token, psm_epaddr_t epaddr,
		      psm_amarg_t *args, int nargs, 
		      void *src, uint32_t len) {
  exit(1);
  int off;
  for( off = 0; off < nargs; off += 2 ) {
    uint64_t * dest_addr = (uint64_t *) args[0+off].u64;
    uint64_t data = args[1+off].u64;
    if (DEBUG) { printf("Node %d got response of %p for %p\n", mynode, data, dest_addr); }
    *dest_addr = data;
    ++reply_received;
  }
}


int AM_NODE_REQUEST_AGG_BULK_HANDLER = -1;
int am_node_request_agg_bulk(psm_am_token_t token, psm_epaddr_t epaddr,
			     psm_amarg_t *args, int nargs, 
			     void *src, uint32_t len) {
  uint64_t * uintargs = (uint64_t *) src;
  stack_element_t sel;
  int off;
  for( off = 0; off * sizeof(uint64_t) < len; off += 2 ) {
    sel.d1 = uintargs[off+0];
    sel.d2 = uintargs[off+1];
    if (DEBUG) { printf("Node %d got request for %p for %p\n", mynode, sel.d1, sel.d2); }
    //if( stack_full( &s ) ) { printf("Node %d pushing ont a full stack\n", mynode); fflush(stdout); exit(2); }
    //stack_push( &s, sel );
    ASSERT_NZ( MCRingBuffer_produce( &q, sel.d1 ) );
    ASSERT_NZ( MCRingBuffer_produce( &q, sel.d2 ) );
    ASSERT_NZ( sel.d1 != 8 );
    //printf("Node %d produced values %p for %p\n", mynode, sel.d1, sel.d2);
    ++request_received;
  }
  return 0;
}

int AM_NODE_REPLY_AGG_BULK_HANDLER = -1;
int am_node_reply_agg_bulk(psm_am_token_t token, psm_epaddr_t epaddr,
			   psm_amarg_t *args, int nargs, 
			   void *src, uint32_t len) {
  uint64_t * uintargs = (uint64_t *) src;
  int off;
  for( off = 0; off * sizeof(uint64_t) < len; off += 2 ) {
    uint64_t * dest_addr = (uint64_t *) uintargs[0+off];
    uint64_t data = uintargs[1+off];
    if (DEBUG) { printf("Node %d got response of %p for %p\n", mynode, data, dest_addr); }
    *dest_addr = data;
    ++reply_received;
  }
}

int AM_QUIT_HANDLER = -1;
int am_quit(psm_am_token_t token, psm_epaddr_t epaddr,
	    psm_amarg_t *args, int nargs, 
	    void *src, uint32_t len) {
  if (DEBUG) { printf("Node %d got quit request\n", mynode); }
  network_done = 1;
  return 0;
}

void setup_ams( psm_ep_t ep ) {
  psm_am_handler_fn_t handlers[] = { &am_node_request, &am_node_reply, 
				     &am_node_request_agg, &am_node_reply_agg, 
				     &am_node_request_agg_bulk, &am_node_reply_agg_bulk, 
				     &am_quit };
  int handler_indices[ sizeof(handlers) ];
  PSM_CHECK( psm_am_register_handlers( ep, handlers, sizeof(handlers), handler_indices ) );
  printf("AM_NODE_REQUEST_HANDLER = %d\n", (AM_NODE_REQUEST_HANDLER = handler_indices[0]) );
  printf("AM_NODE_REPLY_HANDLER = %d\n", (AM_NODE_REPLY_HANDLER = handler_indices[1]) );
  printf("AM_NODE_REQUEST_AGG_HANDLER = %d\n", (AM_NODE_REQUEST_AGG_HANDLER = handler_indices[2]) );
  printf("AM_NODE_REPLY_AGG_HANDLER = %d\n", (AM_NODE_REPLY_AGG_HANDLER = handler_indices[3]) );
  printf("AM_NODE_REQUEST_AGG_BULK_HANDLER = %d\n", (AM_NODE_REQUEST_AGG_BULK_HANDLER = handler_indices[4]) );
  printf("AM_NODE_REPLY_AGG_BULK_HANDLER = %d\n", (AM_NODE_REPLY_AGG_BULK_HANDLER = handler_indices[5]) );
  printf("AM_QUIT_HANDLER = %d\n", (AM_QUIT_HANDLER = handler_indices[6]) );
  PSM_CHECK( psm_am_activate(ep) );
}



void request( void * addr, void * dest_addr ) {
  exit(1);
  psm_amarg_t args[2];
  args[0].u64 = (uint64_t) addr;
  args[1].u64 = (uint64_t) dest_addr;
  if (DEBUG) { printf("Node %d sending request for %p for %p\n", mynode, addr, dest_addr); }
  PSM_CHECK( psm_am_request_short( dest2epaddr( addr2node( addr ) ), AM_NODE_REQUEST_HANDLER, 
				   args, 2, NULL, 0,
				   PSM_AM_FLAG_ASYNC | PSM_AM_FLAG_NOREPLY,
				   NULL, NULL ) );
}

void reply( uint64_t dest_addr, uint64_t data ) {
  exit(1);
  psm_amarg_t args[2];
  args[0].u64 = dest_addr;
  args[1].u64 = data;
  if (DEBUG) { printf("Node %d sending reply for %p for %p\n", mynode, dest_addr, data); }
  PSM_CHECK( psm_am_request_short( dest2epaddr( addr2node( (void*) dest_addr ) ), AM_NODE_REPLY_HANDLER, 
				   args, 2, NULL, 0,
				   PSM_AM_FLAG_ASYNC | PSM_AM_FLAG_NOREPLY,
				   NULL, NULL ) );
}

int aggregation_size = 20;

int request_off = 0;
int request_sent = 0;
psm_amarg_t request_args[ 128 ];
inline void request_agg( void * addr, void * dest_addr ) {
  exit(1);
  request_args[0+request_off].u64 = (uint64_t) addr;
  request_args[1+request_off].u64 = (uint64_t) dest_addr;
  request_off += 2;
  ++request_sent;
  if (DEBUG) { printf("Node %d sending request for %p for %p\n", mynode, addr, dest_addr); }
  if (request_off == aggregation_size * 2) {
    PSM_CHECK( psm_am_request_short( dest2epaddr( addr2node( addr ) ), AM_NODE_REQUEST_AGG_HANDLER, 
				     request_args, aggregation_size * 2, NULL, 0,
				     //PSM_AM_FLAG_ASYNC | PSM_AM_FLAG_NOREPLY,
				     PSM_AM_FLAG_NOREPLY,
				     NULL, NULL ) );
    request_off = 0;
  }
}

inline void request_agg_bulk( void * addr, void * dest_addr ) {
  if (addr != 0) {
    request_args[0+request_off].u64 = (uint64_t) addr;
    request_args[1+request_off].u64 = (uint64_t) dest_addr;
    request_off += 2;
    ASSERT_NZ( addr != 8 );
    ++request_sent;
    if (DEBUG) { printf("Node %d sending request for %p for %p\n", mynode, addr, dest_addr); }
  }
  if( addr == 0 || request_off == aggregation_size * 2 ) {
    PSM_CHECK( psm_am_request_short( dest2epaddr( addr2node( addr ) ), AM_NODE_REQUEST_AGG_BULK_HANDLER, 
				     //request_args, aggregation_size * 2, NULL, 0,
				     NULL, 0, request_args, request_off * sizeof(uint64_t),
				     //PSM_AM_FLAG_ASYNC | PSM_AM_FLAG_NOREPLY,
				     PSM_AM_FLAG_NOREPLY,
				     NULL, NULL ) );
    request_off = 0;
  }
}

int reply_off = 0;
int reply_sent = 0;
psm_amarg_t reply_args[ 128 ];
inline void reply_agg( uint64_t dest_addr, uint64_t data ) {
  exit(1);
  reply_args[0+reply_off].u64 = dest_addr;
  reply_args[1+reply_off].u64 = data;
  reply_off += 2;
  ++reply_sent;
  if (DEBUG) { printf("Node %d sending reply for %p for %p\n", mynode, dest_addr, data); }
  if( reply_off == aggregation_size * 2 ) {
    PSM_CHECK( psm_am_request_short( dest2epaddr( addr2node( (void*) dest_addr ) ), AM_NODE_REPLY_AGG_HANDLER, 
				     reply_args, aggregation_size * 2, NULL, 0,
				     //PSM_AM_FLAG_ASYNC | PSM_AM_FLAG_NOREPLY,
				     PSM_AM_FLAG_NOREPLY,
				     NULL, NULL ) );
    reply_off = 0;
  }
}

inline void reply_agg_bulk( uint64_t dest_addr, uint64_t data ) {
  if (dest_addr != 0) {
    reply_args[0+reply_off].u64 = dest_addr;
    reply_args[1+reply_off].u64 = data;
    reply_off += 2;
    ++reply_sent;
    if (DEBUG) { printf("Node %d sending reply for %p for %p\n", mynode, dest_addr, data); }
  }
  if( dest_addr == 0 || reply_off == aggregation_size * 2 ) {
    PSM_CHECK( psm_am_request_short( dest2epaddr( addr2node( (void*) dest_addr ) ), AM_NODE_REPLY_AGG_BULK_HANDLER, 
				     //reply_args, aggregation_size * 2, NULL, 0,
				     NULL, 0, reply_args, reply_off * sizeof(uint64_t),
				     //PSM_AM_FLAG_ASYNC | PSM_AM_FLAG_NOREPLY,
				     PSM_AM_FLAG_NOREPLY,
				     NULL, NULL ) );
    reply_off = 0;
  }
}

void quit() {
  if (DEBUG) { printf("Node %d sending quit request\n", mynode); }
  PSM_CHECK( psm_am_request_short( dest2epaddr( mycounterpart() ), AM_QUIT_HANDLER, 
				   NULL, 0, NULL, 0,
				   PSM_AM_FLAG_ASYNC | PSM_AM_FLAG_NOREPLY,
				   NULL, NULL ) );
}


struct mailbox {
  uint64_t data;
  uint64_t addr;
  int full;
};

inline void issue( struct node ** pointer, struct mailbox * mb ) {
  if( islocal( pointer ) ) {
    __builtin_prefetch( pointer, 0, 0 );
  } else {
    mb->data = -1;
    mb->addr = (uint64_t) pointer;
    request_agg_bulk( pointer, &mb->data );
  }
}

inline struct node * complete( thread * me, struct node ** pointer, struct mailbox * mb ) {
  if( islocal( pointer ) ) {
    return *pointer;
  } else {
    while( -1 == mb->data ) {
      thread_yield( me );
    }
    return (struct node *) mb->data;
  }
}

uint64_t walk_prefetch_switch( thread * me, node * current, uint64_t count ) {
  struct mailbox mb;
  mb.data = -1;
  mb.addr = -1;
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
  if (DEBUG) { printf("Node %d thread starting\n", mynode); fflush(stdout); }
  args->result = walk_prefetch_switch( me, args->base, args->count );
  --local_active;
  if( local_active == 0 ) {
    local_done = 1;
    if (DEBUG) { printf("Node %d Calling quit\n", mynode); fflush(stdout); }
    quit();
    if (DEBUG) { printf("Node %d Quit called\n", mynode); fflush(stdout); }
  }
  if (DEBUG) { printf("returned %lx\n", args->result); }
}


struct network_args {
  stack_t * s;
};

int request_processed;
void network_runnable( thread * me, void * argsv ) {
  struct network_args * args = argsv;
  psm_ep_t ep = get_ep();
  if (DEBUG) { printf("Node %d network thread starting\n", mynode); fflush(stdout); }
  //while( network_done == 0 || local_done == 0 || !stack_empty( &s ) ) {
  while( network_done == 0 || local_done == 0 ) {
    //printf("node %d network_done = %d local_done = %d\n", mynode, network_done, local_done );
    int prev = s.index;
    psm_poll(ep);
    int post = s.index;
    if ( post < prev) { printf("Node %d prev %d post %d\n", mynode, prev, post); fflush(stdout); }

    if (0) {
      if( !stack_empty( &s ) ) {
	stack_element_t sel = stack_get( &s );
	uint64_t * addr = (uint64_t *) sel.d1;
	stack_pop( &s );
	++request_processed;

	__builtin_prefetch( addr, 0, 0 );
	
	thread_yield( me );
	
	reply_agg( sel.d2, *addr );
      } else {
	thread_yield( me );
      }
    }

    if (0) {
      if( !stack_empty( &s ) ) {
	int x;
	int max_concurrent = 5;
	int current_concurrent = 0;
	uint64_t * addrs[max_concurrent];
	uint64_t dests[max_concurrent];

	for( x = 0; x < max_concurrent && !stack_empty( &s ); ++x ) {
	  stack_element_t sel = stack_get( &s );
	  stack_pop( &s );
	  addrs[ x ] = (uint64_t *) sel.d1;
	  __builtin_prefetch( addrs[x], 0, 0 );
	  dests[ x ] = sel.d2;
	  ++current_concurrent;
	  ++request_processed;
	}

	thread_yield(me);

	for( x = 0; x < current_concurrent; ++x ) {
	  reply_agg_bulk( dests[x], *addrs[x] );
	}
      } else {
	thread_yield(me);
      }
    }

    if (0) {
      if( !stack_empty( &s ) ) {
	while( !stack_empty( &s ) ) {
	  stack_element_t sel = stack_get( &s );
	  uint64_t * addr = (uint64_t *) sel.d1;

	  stack_pop( &s );
	  ++request_processed;
	  __builtin_prefetch( addr, 0, 0 );
	  
	  thread_yield( me );
	  
	  reply_agg_bulk( sel.d2, *addr );
	}
      } else {
	thread_yield( me );
      }
    }

    if (0) {
      uint64_t w1 = 0;
      uint64_t w2 = 0;
      while( MCRingBuffer_consume( &q, &w1 ) ) {
	//printf("Node %d consumed value %p\n", mynode, w1);
	ASSERT_NZ( MCRingBuffer_consume( &q, &w2 ) );
	//printf("Node %d consumed value %p\n", mynode, w2);
	uint64_t * addr = (uint64_t *) w1;
	++request_processed;
	__builtin_prefetch( addr, 0, 0 );
	
	thread_yield( me );
	
	reply_agg_bulk( w2, *addr );
      } 
      thread_yield( me );
    }

    if (1) {
      uint64_t w1 = 0;
      uint64_t w2 = 0;
      while( MCRingBuffer_consume( &q, &w1 ) ) {
	
	volatile int x = 0;
	int max_concurrent = 10;
	int current_concurrent = 0;
	uint64_t * addrs[max_concurrent];
	uint64_t dests[max_concurrent];
	
	do {
	  ASSERT_NZ( w1 != 8 );
	  ASSERT_NZ( MCRingBuffer_consume( &q, &w2 ) );

	  addrs[ x ] = (uint64_t *) w1;
	  __builtin_prefetch( addrs[x], 0, 0 );
	  dests[ x ] = w2;
	  ++current_concurrent;
	  ++request_processed;
	  ++x;
	} while( x < max_concurrent && MCRingBuffer_consume( &q, &w1 ) );

	thread_yield(me);

	for( x = 0; x < current_concurrent; ++x ) {
	  volatile uint64_t thisdest = dests[x];
	  volatile uint64_t thisaddr = addrs[x];
	  volatile uint64_t newdata = *(addrs[x]);
	  if (newdata < 100) { printf("Node %d got data %p for address %p\n", mynode, newdata, thisaddr); fflush(stdout); }
	  reply_agg_bulk( thisdest, newdata );
	}
      }
      thread_yield(me);
    }

    
    
  }
  if (DEBUG) { printf("Node %d network thread done\n", mynode); fflush(stdout); }
}



void flusher_runnable( thread * me, void * argsv ) {
  while( network_done == 0 || local_done == 0 ) {
    MCRingBuffer_flush( &q );
    if (request_off != 0) request_agg_bulk( 0, 0 );
    if (reply_off != 0) reply_agg_bulk( 0, 0 );
    thread_yield( me );
  }
}



int main( int argc, char * argv[] ) {

  //ASSERT_Z( gasnet_init(&argc, &argv) );

  gasnet_node_t gasneti_mynode = -1;
  gasnet_node_t gasneti_nodes = 0;

  gasneti_bootstrapInit_ssh( &argc, &argv, &gasneti_nodes, &gasneti_mynode );
  printf("initialized node %d of %d on %s pd %d\n", gasneti_mynode, gasneti_nodes, gasneti_gethostname(), getpid() );

  mynode = gasneti_mynode;
  num_nodes = gasneti_nodes;
  
  psm_setup( gasneti_mynode, gasneti_nodes);
  setup_ams( get_ep() );
  printf("Node %d PSM is set up.\n", mynode); fflush(stdout);

  /* struct psm_am_max_sizes sizes; */
  /* PSM_CHECK(  psm_am_get_max_sizes( get_ep(), &sizes ) ); */
  /* printf("Node %d has PSM nargs=%u, request_short=%u, reply_short=%u\n", mynode, sizes.nargs, sizes.request_short, sizes.reply_short); fflush(stdout); */

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
  //bases = allocate_heap(opt->list_size, opt->cores, opt->threads, &nodes);
  printf("Node %d base address is %p, extent is %p.\n", mynode, nodes, nodes + sizeof(node) * opt->list_size); fflush(stdout);

  int i, core_num;

  ASSERT_NZ( opt->cores == 1 );
  //ASSERT_NZ( opt->threads == 1 );
  for( i = 0; i < opt->list_size; ++i ) {
    if( nodes[i].next == NULL ) printf("Node %d found NULL next at element %d\n", mynode, i );
    ASSERT_NZ( nodes[i].next );
  }

  //  sleep(10);



  thread * threads[ opt->cores ][ opt->threads ];
  thread * masters[ opt->cores ];
  scheduler * schedulers[ opt->cores ];

  struct thread_args args[ opt->cores ][ opt->threads ];

  struct network_args netargs;
  stack_init( &s, 1024 );
  netargs.s = &s;
  MCRingBuffer_init( &q );
  gasneti_bootstrapBarrier_ssh();

  printf("Node %d spawning threads.\n", mynode); fflush(stdout);
  //#pragma omp parallel for num_threads( opt->cores )
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
      if ( !(thread_num & 0xf) ) thread_spawn( masters[ core_num ], schedulers[ core_num ], network_runnable, &netargs );
      //thread_spawn( masters[ core_num ], schedulers[ core_num ], network_runnable, &netargs );
    }
    thread_spawn( masters[ core_num ], schedulers[ core_num ], flusher_runnable, NULL );
  }


  struct timespec start_time;
  struct timespec end_time;

  gasneti_bootstrapBarrier_ssh();
  printf("Node %d starting.\n", mynode); fflush(stdout);
  clock_gettime(CLOCK_MONOTONIC, &start_time);
  //#pragma omp parallel for num_threads( opt->cores )
  for( core_num = 0; core_num < opt->cores; ++core_num ) {
    run_all( schedulers[ core_num ] );
  }
  clock_gettime(CLOCK_MONOTONIC, &end_time);
  printf("Node %d done.\n", mynode); fflush(stdout);
  gasneti_bootstrapBarrier_ssh();


  uint64_t sum;
  //#pragma omp parallel for num_threads( opt->cores ) reduction( + : sum )
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
