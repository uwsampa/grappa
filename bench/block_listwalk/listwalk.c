#define _GNU_SOURCE
#define __USE_GNU
#include <sched.h>
#include <sys/time.h>
#include <sys/resource.h>

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

#include "metrics.h"

#include <MCRingBuffer.h>



#define rdtscll(val) do { \
  unsigned int __a,__d; \
  asm volatile("rdtsc" : "=a" (__a), "=d" (__d)); \
  (val) = ((unsigned long)__a) | (((unsigned long)__d)<<32); \
  } while(0)


static gasnet_node_t mynode = -1;
static gasnet_node_t num_nodes = 0;



typedef struct stack_element {
  uint64_t d1;
  uint64_t d2;
} stack_element_t;

MCRingBuffer q;

int everything_is_local = 0;
inline int islocal( void * addr ) {
  return everything_is_local;
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

struct mailbox {
  uint64_t data;
  uint64_t addr;
  uint64_t issue_ts;
  uint64_t send_ts;
  uint64_t recv_ts;
  uint64_t network_ts;
  uint64_t recvd_ts;
  uint64_t complete_ts;
  int full;
  uint64_t issue_count;
  uint64_t send_count;
  uint64_t reply_count;
  uint64_t * payload;
  uint64_t * local_payload;
  uint64_t payload_size;
};




int network_done = 0;
int local_done = 0;
int local_active = 0;
int request_received = 0;
int reply_count = 0;

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
    ASSERT_NZ( MCRingBuffer_produce( &q, sel.d1 ) );
    ASSERT_NZ( MCRingBuffer_produce( &q, sel.d2 ) );
    if (DEBUG) { printf("Node %d produced values %p for %p\n", mynode, sel.d1, sel.d2); }
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
  uint64_t recv_ts;
  rdtscll( recv_ts );
  for( off = 0; off * sizeof(uint64_t) < len; off += 2 ) {
    //uint64_t * dest_addr = (uint64_t *) uintargs[0+off];
    struct mailbox * mb = (struct mailbox *) uintargs[0+off];
    uint64_t data = uintargs[1+off];
    if (DEBUG) { printf("Node %d got response of %p for mailbox %p\n", mynode, data, mb); }
    mb->data = data;
    mb->recv_ts = recv_ts;
    ++reply_count;
  }
}


uint64_t prev_addr = 0;
uint64_t prev_data = 0;

int AM_NODE_REPLY_AGG_BULK_BLOCK_HANDLER = -1;
int am_node_reply_agg_bulk_block(psm_am_token_t token, psm_epaddr_t epaddr,
				 psm_amarg_t *args, int nargs, 
				 void *src, uint32_t len) {
  exit(99);
  uint64_t * uintargs = (uint64_t *) src;
  int off;
  uint64_t recv_ts;
  rdtscll( recv_ts );
    uint64_t message_payload_len = 0;
  for( off = 0; off * sizeof(uint64_t) < len; off += 3 + message_payload_len ) {
    //uint64_t * dest_addr = (uint64_t *) uintargs[0+off];
    struct mailbox * mb = (struct mailbox *) uintargs[0+off];
    uint64_t data = uintargs[1+off];
    message_payload_len = uintargs[2+off];
    uint64_t * message_payload = &uintargs[3+off];
    if (1 || DEBUG) { printf("Node %d got reply for mailbox %p data %p len %p new offset %lu\n", mynode, mb, data, message_payload_len, off); }  
    mb->reply_count++;
    if (0) {
      if( mb->issue_count < mb->reply_count ) {
	printf("Node %d got duplicate mailbox address of %p with data %p (old data %p) at offset %d after %lu replies (mailbox issue count %lu send count %lu reply count %lu) prev_addr %p prev_data %p\n", mynode, mb, uintargs[1+off], mb->data, off, reply_count, mb->issue_count, mb->send_count, mb->reply_count, prev_addr, prev_data);
	assert(0);
      }
      assert(data != -1);
    }
    if (DEBUG) { printf("Node %d got response of %p for mailbox %p\n", mynode, data, mb); }
    mb->data = data;
    mb->recv_ts = recv_ts;
    uint64_t * tsarg = (uint64_t *) args;
    mb->network_ts = tsarg[0];
    ++reply_count;
    prev_addr = uintargs[0+off];
    prev_data = uintargs[1+off];
    memcpy( mb->local_payload, message_payload, message_payload_len * sizeof(uint64_t) );
  }
  assert( off * sizeof(uint64_t) == len );
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
  psm_am_handler_fn_t handlers[] = { &am_node_request_agg_bulk, &am_node_reply_agg_bulk, 
				     &am_node_reply_agg_bulk_block, 
				     &am_quit };
  int handler_indices[ sizeof(handlers) ];
  PSM_CHECK( psm_am_register_handlers( ep, handlers, sizeof(handlers), handler_indices ) );
  printf("AM_NODE_REQUEST_AGG_BULK_HANDLER = %d\n", (AM_NODE_REQUEST_AGG_BULK_HANDLER = handler_indices[0]) );
  printf("AM_NODE_REPLY_AGG_BULK_HANDLER = %d\n", (AM_NODE_REPLY_AGG_BULK_HANDLER = handler_indices[1]) );
  printf("AM_NODE_REPLY_AGG_BULK_BLOCK_HANDLER = %d\n", (AM_NODE_REPLY_AGG_BULK_BLOCK_HANDLER = handler_indices[2]) );
  printf("AM_QUIT_HANDLER = %d\n", (AM_QUIT_HANDLER = handler_indices[3]) );
  PSM_CHECK( psm_am_activate(ep) );
}



int aggregation_size = 20;

int request_off = 0;
int request_sent = 0;
psm_amarg_t request_args[ 128 ];
struct mailbox * mailboxes[ 128 ];
uint64_t send_count = 0;

inline void request_agg_bulk( void * addr, void * dest_addr, struct mailbox * mb ) {
  if (addr != 0) {
    request_args[0+request_off].u64 = (uint64_t) addr;
    request_args[1+request_off].u64 = (uint64_t) dest_addr;
    mailboxes[request_off / 2] = mb;
    request_off += 2;
    ++request_sent;
    if (DEBUG) { printf("Node %d sending request for %p for %p\n", mynode, addr, dest_addr); }
  }
  if( (addr == 0 && request_off > 0) || request_off == aggregation_size * 2 ) {
    int i;
    uint64_t current_ts;
    ++send_count;
    rdtscll( current_ts );
    for( i = 0; i < request_off / 2; ++i ) {
      mailboxes[i]->send_ts = current_ts;
    }
    PSM_CHECK( psm_am_request_short( dest2epaddr( addr2node( addr ) ), AM_NODE_REQUEST_AGG_BULK_HANDLER, 
				     NULL, 0, request_args, request_off * sizeof(uint64_t),
				     PSM_AM_FLAG_NOREPLY,
				     NULL, NULL ) );
    request_off = 0;
  }
}

int reply_off = 0;
int reply_sent = 0;
psm_amarg_t reply_args[ 128 ];

inline void reply_agg_bulk( uint64_t dest_addr, uint64_t data ) {
  if (dest_addr != 0) {
    reply_args[0+reply_off].u64 = dest_addr;
    reply_args[1+reply_off].u64 = data;
    reply_off += 2;
    ++reply_sent;
    if (DEBUG) { printf("Node %d sending reply for %p for %p\n", mynode, dest_addr, data); }
  }
  if( (dest_addr == 0 && reply_off > 0) || reply_off == aggregation_size * 2 ) {
    PSM_CHECK( psm_am_request_short( dest2epaddr( addr2node( (void*) dest_addr ) ), AM_NODE_REPLY_AGG_BULK_HANDLER, 
				     NULL, 0, reply_args, reply_off * sizeof(uint64_t),
				     PSM_AM_FLAG_NOREPLY,
				     NULL, NULL ) );
    reply_off = 0;
  }
}



uint64_t block_payload_len = 0;
uint64_t * block_payload = 0;
uint64_t batch_size = 0;
inline void reply_agg_bulk_block( uint64_t dest_addr, uint64_t data, uint64_t ts ) {
  exit(98);
  if (dest_addr != 0) {
    reply_args[0+reply_off].u64 = dest_addr;
    reply_args[1+reply_off].u64 = data;
    reply_args[2+reply_off].u64 = block_payload_len;
    memcpy( &reply_args[3+reply_off].u64, block_payload, block_payload_len * sizeof(uint64_t) );
    reply_off += 3 + block_payload_len;
    ++reply_sent;
    ++batch_size;
    if (1 || DEBUG) { printf("Node %d sending reply for %p for %p new offset %lu\n", mynode, dest_addr, data, reply_off); }  
  }
  if( (dest_addr == 0 && reply_off > 0)  || batch_size == aggregation_size ) {
    PSM_CHECK( psm_am_request_short( dest2epaddr( addr2node( (void*) dest_addr ) ), AM_NODE_REPLY_AGG_BULK_BLOCK_HANDLER, 
				     NULL, 0, reply_args, reply_off * sizeof(uint64_t),
				     PSM_AM_FLAG_NOREPLY,
				     NULL, NULL ) );
    reply_off = 0;
    batch_size = 0;
  }
}


void quit() {
  if (DEBUG) { printf("Node %d sending quit request\n", mynode); }
  PSM_CHECK( psm_am_request_short( dest2epaddr( mycounterpart() ), AM_QUIT_HANDLER, 
				   NULL, 0, NULL, 0,
				   PSM_AM_FLAG_ASYNC | PSM_AM_FLAG_NOREPLY,
				   NULL, NULL ) );
}


uint64_t total_latency_ticks = 0;
uint64_t aggregation_latency_ticks = 0;
uint64_t network_latency_ticks = 0;
uint64_t wakeup_latency_ticks = 0;
inline void collect_mailbox_timestamps( struct mailbox * mb ) {
  total_latency_ticks += mb->complete_ts - mb->issue_ts;
  aggregation_latency_ticks += mb->send_ts - mb->issue_ts;
  network_latency_ticks += mb->recv_ts - mb->send_ts;
  wakeup_latency_ticks += mb->complete_ts - mb->recv_ts;
}

uint64_t issue_count = 0;
inline void issue( struct node ** pointer, struct mailbox * mb ) {
  //  ++issue_count;
  rdtscll( mb->issue_ts );
  if( islocal( pointer ) ) {
    __builtin_prefetch( pointer, 0, 0 );
    rdtscll( mb->send_ts );
  } else {
    mb->data = -1;
    mb->addr = (uint64_t) pointer;
    request_agg_bulk( pointer, &mb->data, mb );
  }
}

inline struct node * complete( thread * me, struct node ** pointer, struct mailbox * mb ) {
  struct node * node;
  if( islocal( pointer ) ) {
    //return *pointer;
    node = *pointer;
    rdtscll( mb->recv_ts );
  } else {
    while( -1 == mb->data ) {
      thread_yield( me );
    }
    node = (struct node *) mb->data;
  }
  rdtscll( mb->complete_ts );
  collect_mailbox_timestamps(mb);
  return node;
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

uint64_t walk_prefetch_switch_sum_inline( thread * me, node * current, uint64_t count, uint64_t * payload, uint64_t payload_size ) {
  struct mailbox mb;
  mb.data = -1;
  mb.addr = -1;
  mb.full = 0;
  mb.issue_count = 0;
  mb.send_count = 0;
  mb.reply_count = 0;
  uint64_t sum = 0;

  while( count > 0 ) {
    --count;
    issue( &current->next, &mb );
    thread_yield( me );
    current = complete( me, &current->next, &mb );

    uint64_t i;
    for( i = 0; i < payload_size; ++i ) {
      issue( (node **) &payload[i], &mb );
      thread_yield( me );
      sum += (uint64_t) complete( me, (node **) &payload[i], &mb );
    }
    
  }
  return (uint64_t) current + sum;
}


uint64_t walk_prefetch_switch_sum_prefetch( thread * me, node * current, uint64_t count, uint64_t * payload, uint64_t payload_size ) {
  struct mailbox mb[payload_size + 1];
  int i;
  for( i = 0; i < payload_size + 1; ++i ) {
    mb[i].data = -1;
    mb[i].addr = -1;
    mb[i].full = 0;
    mb[i].issue_count = 0;
    mb[i].send_count = 0;
    mb[i].reply_count = 0;
  }
  uint64_t sum = 0;

  while( count > 0 ) {
    --count;
    issue( &current->next, &mb[0] );
    for( i = 0; i < payload_size; ++i ) {
      issue( (node **) &payload[i], &mb[i+1] );
    }
    thread_yield( me );
    current = complete( me, &current->next, &mb[0] );
    for( i = 0; i < payload_size; ++i ) {
      sum += (uint64_t) complete( me, (node **) &payload[i], &mb[i+1] );
    }

  }
  return (uint64_t) current + sum;
}


uint64_t walk_prefetch_switch_sum_block( thread * me, node * current, uint64_t count, uint64_t * payload, uint64_t payload_size ) {
  struct mailbox mb;
  int i;
  mb.data = -1;
  mb.addr = -1;
  mb.full = 0;
  mb.issue_count = 0;
  mb.send_count = 0;
  mb.reply_count = 0;
  mb.local_payload = payload; 
  uint64_t sum = 0;

  while( count > 0 ) {
    --count;
    issue( &current->next, &mb );
    thread_yield( me );
    current = complete( me, &current->next, &mb );
    for( i = 0; i < payload_size; ++i ) {
      sum += mb.local_payload[i];
    }

  }

  return (uint64_t) current + sum;
}



struct thread_args {
  node * base;
  uint64_t count;
  uint64_t result;
  char * type;
  uint64_t * payload;
  uint64_t payload_size;
};

void thread_runnable( thread * me, void * argsv ) {
  struct thread_args * args = argsv;
  if (DEBUG) { printf("Node %d thread starting\n", mynode); fflush(stdout); }
  if( args->type && 0 == strcmp( args->type, "block" ) ) {
    args->result = walk_prefetch_switch_sum_block( me, args->base, args->count, args->payload, args->payload_size );    
  } else if( args->type && 0 == strcmp( args->type, "prefetch" ) ) {
    args->result = walk_prefetch_switch_sum_prefetch( me, args->base, args->count, args->payload, args->payload_size );    
  } else if( args->type && 0 == strcmp( args->type, "inline" ) ) {
    args->result = walk_prefetch_switch_sum_inline( me, args->base, args->count, args->payload, args->payload_size );    
  } else {
    args->result = walk_prefetch_switch( me, args->base, args->count );
  }
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
};

int request_processed;
int64_t polling_ticks = 0;
uint64_t poll_count = 0;

int64_t service_ticks = 0;
uint64_t service_count = 0;
void network_runnable( thread * me, void * argsv ) {
  struct network_args * args = argsv;
  psm_ep_t ep = get_ep();
  if (DEBUG) { printf("Node %d network thread starting\n", mynode); fflush(stdout); }
  int64_t ts;
  rdtscll(ts);
  polling_ticks -= ts;

  while( network_done == 0 || local_done == 0 ) {

    psm_poll(ep);
    ++poll_count;

    if (1) {
      uint64_t w1 = 0;
      uint64_t w2 = 0;
      while( MCRingBuffer_consume( &q, &w1 ) ) {
	uint64_t start_ts;
	rdtscll(start_ts);

	volatile int x = 0;
	int max_concurrent = 5;
	int current_concurrent = 0;
	uint64_t * addrs[max_concurrent];
	uint64_t dests[max_concurrent];
	
	do {
	  MCRingBuffer_consume( &q, &w2 );

	  addrs[ x ] = (uint64_t *) w1;
	  __builtin_prefetch( addrs[x], 0, 0 );
	  dests[ x ] = w2;
	  ++current_concurrent;
	  ++request_processed;
	  ++x;
	} while( x < max_concurrent && MCRingBuffer_consume( &q, &w1 ) );

	rdtscll(ts);
	polling_ticks += ts;
	thread_yield( me );
	rdtscll(ts);
	polling_ticks -= ts;

	for( x = 0; x < current_concurrent; ++x ) {
	  volatile uint64_t thisdest = dests[x];
	  volatile uint64_t thisaddr = (uint64_t) addrs[x];
	  volatile uint64_t newdata = *(addrs[x]);
	  reply_agg_bulk( thisdest, newdata );
	}
	uint64_t end_ts;
	rdtscll(end_ts);
	service_ticks += end_ts - start_ts;
	++service_count;
      }
      rdtscll(ts);
      polling_ticks += ts;
      thread_yield( me );
      rdtscll(ts);
      polling_ticks -= ts;
    }

    
    
  }
  rdtscll(ts);
  polling_ticks += ts;
  if (DEBUG) { printf("Node %d network thread done\n", mynode); fflush(stdout); }
}



void flusher_runnable( thread * me, void * argsv ) {
  while( network_done == 0 || local_done == 0 ) {
    MCRingBuffer_flush( &q );
    if (request_off != 0) request_agg_bulk( 0, 0, NULL);
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

  cpu_set_t   mask;
  CPU_ZERO(&mask);
  int mycore = mynode % (num_nodes / 2);
  CPU_SET(mycore, &mask);
  sched_setaffinity(0, sizeof(mask), &mask);
  
  psm_setup( gasneti_mynode, gasneti_nodes);
  setup_ams( get_ep() );
  printf("Node %d PSM is set up.\n", mynode); fflush(stdout);

  //ASSERT_Z( gasnet_attach( NULL, 0, 0, 0 ) );
  //printf("hello from node %d of %d\n", gasnet_mynode(), gasnet_nodes() );

  struct options opt_actual = parse_options( &argc, &argv );
  struct options * opt = &opt_actual;

  aggregation_size = opt->batch_size;
  everything_is_local = opt->force_local;

  printf("Node %d allocating heap.\n", mynode); fflush(stdout);
  node * nodes = NULL;
  node ** bases = NULL;
  void* nodes_start = (void*) 0x0000111122220000UL;
  nodes = mmap( nodes_start, sizeof(node) * opt->list_size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, 0, 0 );
  assert( nodes == nodes_start );

  allocate_heap(opt->list_size, opt->cores, opt->threads, &bases, &nodes);

  printf("Node %d base address is %p, extent is %p.\n", mynode, nodes, nodes + sizeof(node) * opt->list_size); fflush(stdout);

  int i, core_num;
  
  int processes = opt->cores;
  opt->cores = 1;
  ASSERT_NZ( opt->cores == 1 );
  //ASSERT_NZ( opt->threads == 1 );
  for( i = 0; i < opt->list_size; ++i ) {
    if( nodes[i].next == NULL ) printf("Node %d found NULL next at element %d\n", mynode, i );
    ASSERT_NZP( nodes[i].next );
  }

  //  sleep(10);



  thread * threads[ opt->cores ][ opt->threads ];
  thread * masters[ opt->cores ];
  scheduler * schedulers[ opt->cores ];

  struct thread_args args[ opt->cores ][ opt->threads ];

  struct network_args netargs;
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
      args[ core_num ][ thread_num ].type = opt->type;
      args[ core_num ][ thread_num ].payload = NULL;
      args[ core_num ][ thread_num ].payload_size = 0;
      
      if (1) {
	// set up senders
	if (mynode < num_nodes / 2) {
	  threads[ core_num ][ thread_num ] = thread_spawn( masters[ core_num ], schedulers[ core_num ], 
							    thread_runnable, &args[ core_num ][ thread_num ] );
	  ++local_active;
	  network_done = 1;
	} else { 
	  local_done = 1; 
	}
      }

      // set up network service threads
      if (mynode < num_nodes / 2) {
	if ( !(thread_num & 0x7) ) thread_spawn( masters[ core_num ], schedulers[ core_num ], network_runnable, &netargs );
	/* thread_spawn( masters[ core_num ], schedulers[ core_num ], network_runnable, &netargs ); */
      } else {
	/* if ( !(thread_num & 0xffffff) ) thread_spawn( masters[ core_num ], schedulers[ core_num ], network_runnable, &netargs ); */
	/* thread_spawn( masters[ core_num ], schedulers[ core_num ], network_runnable, &netargs ); */
	if( thread_num < 7 ) { // limit ourselves to a few service threads on receiver to minimize latency
	  thread_spawn( masters[ core_num ], schedulers[ core_num ], network_runnable, &netargs );
	}
      }
      //thread_spawn( masters[ core_num ], schedulers[ core_num ], network_runnable, &netargs );
    }
    // set up periodic flusher
    thread_spawn( masters[ core_num ], schedulers[ core_num ], flusher_runnable, NULL );
  }


  struct timespec start_time;
  struct timespec end_time;
  uint64_t start_ts, end_ts;

  gasneti_bootstrapBarrier_ssh();
  printf("Node %d starting.\n", mynode); fflush(stdout);
  clock_gettime(CLOCK_MONOTONIC, &start_time);
  rdtscll(start_ts);
  //#pragma omp parallel for num_threads( opt->cores )
  for( core_num = 0; core_num < opt->cores; ++core_num ) {
    run_all( schedulers[ core_num ] );
  }
  rdtscll(end_ts);
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

  ASSERT_NZ( end_ts > start_ts );
  uint64_t runtime_ticks = end_ts - start_ts;
  double tick_rate = (double) runtime_ticks / runtime;
  double total_latency = (double) total_latency_ticks / tick_rate;
  double aggregation_latency = (double) aggregation_latency_ticks / tick_rate;
  double network_latency = (double) network_latency_ticks / tick_rate;
  double wakeup_latency = (double) wakeup_latency_ticks / tick_rate;
  double polling_latency = (double) polling_ticks / tick_rate;
  double service_latency = (double) service_ticks / tick_rate;

  issue_count = (opt->list_size * opt->count);
  double avg_latency = total_latency / issue_count; //(opt->list_size * opt->count);
  double avg_aggregation_latency = aggregation_latency / issue_count; //(opt->list_size * opt->count);
  double avg_network_latency = network_latency / issue_count; //(opt->list_size * opt->count);
  double avg_wakeup_latency = wakeup_latency / issue_count; //(opt->list_size * opt->count);
  double avg_polling_latency = polling_latency / poll_count; //(opt->list_size * opt->count);
  double avg_service_latency = service_latency / service_count; //(opt->list_size * opt->count);
  

  if( 1 || mynode < num_nodes / 2 ) {
    /* printf("header, id, batch_size, list_size, count, processes, threads, runtime, message rate (M/s), bandwidth (MB/s), sum, runtime_ticks, total_latency_ticks, tick_rate, issue_count, send_count, avg_latency (us), aggregation_latency (us), network_latency (us), wakeup_latency (us), poll_count, polling_latency (us), service_count, service_latency (us)\n"); */
    /* printf("data, %d, %d, %d, %d, %d, %d, %f, %f, %f, %lu, %lu, %lu, %f, %lu, %lu, %.9f, %.9f, %.9f, %.9f, %lu, %.9f, %lu, %.9f\n", opt->id, opt->batch_size, opt->list_size, opt->count, processes, opt->threads, runtime, rate / 1000000, bandwidth / (1024 * 1024), sum, runtime_ticks, total_latency_ticks, tick_rate, issue_count, send_count, avg_latency * 1000000, avg_aggregation_latency * 1000000, avg_network_latency * 1000000, avg_wakeup_latency * 1000000, poll_count, avg_polling_latency * 1000000, service_count, avg_service_latency * 1000000); */

#define METRICS( METRIC )						\
    METRIC( "Node",			"%d",   mynode );		\
    METRIC( "ID",			"%d",   opt->id );		\
    METRIC( "batch_size", 		"%d",   opt->batch_size );	\
    METRIC( "processes", 		"%d",   processes );		\
    METRIC( "threads",			"%d",   opt->threads );		\
    METRIC( "list_size", 		"%d",   opt->list_size );	\
    METRIC( "count", 			"%d",   opt->count );		\
    METRIC( "force_local", 		"%d",   opt->force_local );	\
    METRIC( "is_sender", 		"%d",   mynode < num_nodes / 2 ); \
    METRIC( "runtime",			"%f",   runtime );		\
    METRIC( "message rate (M/s)", 	"%f",   rate / 1000000 );	\
    METRIC( "bandwidth (MB/s)", 	"%f",   bandwidth / (1024 * 1024) ); \
    METRIC( "sum", 			"%lu",  sum );			\
    METRIC( "runtime_ticks", 		"%lu",  runtime_ticks );	\
    METRIC( "total_latency_ticks", 	"%lu",  total_latency_ticks );	\
    METRIC( "tick_rate", 		"%f",   tick_rate );		\
    METRIC( "issue_count", 		"%lu",  issue_count );		\
    METRIC( "send_count", 		"%lu",  send_count );		\
    METRIC( "Average Latency (us)",	"%.9f", avg_latency * 1000000 ); \
    METRIC( "aggregation_latency (us)", "%.9f", avg_aggregation_latency * 1000000 ); \
    METRIC( "network_latency (us)", 	"%.9f", avg_network_latency * 1000000 ); \
    METRIC( "wakeup_latency (us)", 	"%.9f", avg_wakeup_latency * 1000000 ); \
    METRIC( "Poll count",		"%lu",  poll_count );		\
    METRIC( "polling_latency (us)", 	"%.9f", avg_polling_latency * 1000000 ); \
    METRIC( "service_count", 		"%lu",  service_count );	\
    METRIC( "service_latency (us)",     "%.9f", avg_service_latency * 100000 );
    
    PRINT_CSV_METRICS( METRICS );
    PRINT_HUMAN_METRICS( METRICS );
    
  }

  psm_finalize();
  gasneti_bootstrapFini_ssh();
  printf("exiting on node %d on %s\n", gasneti_mynode, gasneti_gethostname() );
  //gasnet_exit(0);
  return 0;
}

