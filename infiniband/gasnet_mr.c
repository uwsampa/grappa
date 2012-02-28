
#define _GNU_SOURCE
#define __USE_GNU
#include <sched.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

#include <mpi.h>
#include <gasnet.h>
#include <stdint.h>
#include <assert.h>

#include "options.h"

//#include "base.h"
//#include "global_memory.h"
//#include "global_array.h"
//#include "msg_aggregator.h"

char data[ 1 << 24 ];
void * global_payload_p = &data[0];

/* #define __USE_GNU */
/* #define _GNU_SOURCE */
/* #include <sched.h> */
/* #include <sys/time.h> */
/* #include <sys/resource.h> */

int masktohex( cpu_set_t * set ) {
  int i;
  int mask = 0;
  for( i = 0; i < 8; ++i ) {
    if( CPU_ISSET( i, set ) ) {
      mask |= 1 << i;
    }
  }
  return mask;
}

static void _set_limits() {
  //struct rlimit r;
  //cpu_set_t   *mask;
  cpu_set_t   mask;
  size_t      size;
  int         ncpus = 16;
  int         i;

  //getrlimit(RLIMIT_NPROC, &r);
  //ncpus = r.rlim_cur;

  //mask = __CPU_ALLOC(ncpus);
  //mask = CPU_ALLOC(ncpus);
  //size = __CPU_ALLOC_SIZE(ncpus);
  //size = CPU_ALLOC_SIZE(ncpus);

  sched_getaffinity(0, sizeof(mask), &mask);
  //printf("CPU_COUNT() of set:    %d\n", CPU_COUNT(mask));

  //__CPU_ZERO_S(size, mask);
  //CPU_ZERO_S(size, mask);
  CPU_ZERO(&mask);

  //printf("CPU_COUNT() of set:    %d\n", CPU_COUNT_S(size, mask));

  for (i = 0; i < ncpus; i++) {
    //__CPU_SET_S(i, size, mask);
    //CPU_SET_S(i, size, mask);
    CPU_SET(i, &mask);
  }
  //printf("CPU_COUNT() of set:    %d\n", CPU_COUNT_S(size, mask));

  //printf("currnet ncpus %d mask %x size %d\n", ncpus, *mask, size);
  sched_setaffinity(0, sizeof(mask), &mask);
  //__CPU_FREE(mask);
  //CPU_FREE(mask);
}

gasnet_seginfo_t    *shared_memory_blocks;

typedef char payload_t;
int payload_size;
payload_t payload;


#define MY_AM 129
void my_am(gasnet_token_t token,
	   gasnet_handlerarg_t a0) {
  //    uint64_t * dest = (uint64_t *) shared_memory_blocks[1].addr;
  payload_t * dest = (payload_t *) shared_memory_blocks[gasnet_mynode()].addr;
  //*dest += a0;
}

#define MY_AM2 130
void my_am2(gasnet_token_t token,
            void * payload, size_t size) {
  //    uint64_t * dest = (uint64_t *) shared_memory_blocks[1].addr;
  //payload_t * dest = (payload_t *) shared_memory_blocks[gasnet_mynode()].addr;
  //*dest += a0;
}



// This crys out for linker sets
gasnet_handlerentry_t   handlers[] =
    {
      { MY_AM, (void *)my_am },
      { MY_AM2, (void *)my_am2 },
        /* { GM_REQUEST_HANDLER, (void *)gm_request_handler }, */
        /* { GM_RESPONSE_HANDLER, (void *)gm_response_handler }, */
        /* { GA_HANDLER, (void *)ga_handler }, */
        /* { MSG_AGGREGATOR_DISPATCH, (void *)msg_aggregator_dispatch_handler }, */
        /* { MSG_AGGREGATOR_REPLY, (void *)msg_aggregator_reply_handler } */
    };

#define SHARED_PROCESS_MEMORY_SIZE  ( 10 * (1 << 20) )
#define SHARED_PROCESS_MEMORY_OFFSET (0)

int messages = 0;

void function_dispatch(int func_id, void *buffer, uint64_t size) {
    ++messages;
    }
    
    
int main(int argc, char **argv) {
    int initialized = 0;
    int i;

    //_set_limits();
    cpu_set_t cpuset;
    CPU_ZERO( &cpuset );
    sched_getaffinity( 0, sizeof(cpuset), &cpuset );
    //assert( CPU_ISSET( i, &cpuset ) );
    printf("main affinity mask is %x\n", masktohex(&cpuset) );


    MPI_Initialized(&initialized);
    if (!initialized)
        if (MPI_Init(&argc, &argv) != MPI_SUCCESS) {
        printf("Failed to initialize MPI\n");
        exit(1);
        }
        
    if (gasnet_init(&argc, &argv) != GASNET_OK) {
        printf("Failed to initialize gasnet\n");
        exit(1);
        }

    char * gasnet_num_qps = gasnet_getenv("GASNET_NUM_QPS");
    printf("GASNET_NUM_QPS = %s\n", gasnet_num_qps);

    struct options opt = parse_options( &argc, &argv );

	int max = opt.count / (gasnet_nodes() / 2);

    if (1) {
    uintptr_t s = gasnet_getMaxLocalSegmentSize();
    printf("Max local segment size is %lu\n", s);
    }

    if (gasnet_attach(handlers,
        sizeof(handlers) / sizeof(gasnet_handlerentry_t),
        SHARED_PROCESS_MEMORY_SIZE,
        SHARED_PROCESS_MEMORY_OFFSET) != GASNET_OK) {
        printf("Failed to allocate sufficient shared memory with gasnet\n");
        gasnet_exit(1);
        }

    shared_memory_blocks = malloc(sizeof(gasnet_seginfo_t) * gasnet_nodes());
    if (gasnet_getSegmentInfo(shared_memory_blocks, gasnet_nodes()) !=
        GASNET_OK) {
        printf("Failed to inquire about shared memory\n");
        gasnet_exit(1);
        }

    if (1) {
    if (gasnet_mynode() == 0) {
        for (i = 0; i < gasnet_nodes(); i++) {
            printf("SHM on %d: at %p of %llx bytes\n",
                i,
                shared_memory_blocks[i].addr,
                (unsigned long long) shared_memory_blocks[i].size);
            }
        }
    }

    
    gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS);
    gasnet_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS);


    payload_size = sizeof(payload_t);
    payload = 0;
    //char data[ opt.payload_size ];
    global_payload_p = &data[0];

    if (gasnet_mynode() >= gasnet_nodes() / 2) {

      int i, j;

      struct timespec starttime;
      struct timespec endtime;
      
    clock_gettime(CLOCK_MONOTONIC, &starttime);

      gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS);
      gasnet_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS);
      
      int dest_node = gasnet_mynode() - gasnet_nodes() / 2;
      if (opt.same_destination) {
	dest_node = 0;
      }

      payload_t * source = (payload_t *) shared_memory_blocks[gasnet_mynode()].addr;
      payload_t * dest = (payload_t *) shared_memory_blocks[dest_node].addr;
    payload = 1;



/* #pragma omp parallel for num_threads( opt.threads ) */
/*     for( i = 0; i < max; ++i )  */
/*       { */
/* 	if (opt.messages) { */
/* 	  gasnet_AMRequestShort1(dest_node, MY_AM, payload ); */
/* 	} else if (opt.rdma_write) { */
/* 	  gasnet_put_nbi_bulk( dest_node, dest, &payload, payload_size ); */
/* 	} else if (opt.rdma_read) { */
/* 	  gasnet_get_nbi_bulk( &payload, dest_node, dest, payload_size ); */
/* 	} */
/*       } */

    int sent = 0;

    //reduction(+ : sent)
#pragma omp parallel for num_threads( opt.threads ) 
    for( i = 0; i < opt.threads; ++i )
      {
/* 	cpu_set_t cpuset; */
/* 	CPU_ZERO( &cpuset ); */
/* 	if (i < 4) */
/* 	  CPU_SET( i*2, &cpuset ); */
/* 	else */
/* 	  CPU_SET( (i - 4)*2 + 1, &cpuset ); */
/* 	CPU_SET( i+1, &cpuset ); */
 	//sched_setaffinity( 0, sizeof(cpuset), &cpuset );
/* 	sched_getaffinity( 0, sizeof(cpuset), &cpuset ); */
	//assert( CPU_ISSET( i, &cpuset ) );
/* 	printf("thread %d affinity mask is %x\n", i, masktohex(&cpuset) ); */
/* 	printf("thread %d max threads %d\n", i, omp_get_max_threads() ); */
/* 	printf("thread %d thread limit %d\n", i, omp_get_thread_limit() ); */
/* 	printf("thread %d number of threads %d\n", i, omp_get_num_threads() ); */
	int frac = max / opt.threads;
	int base = frac * i;

/* 	for( j = base; j < base + frac; ++j ) { */
/* 	  if (opt.messages) { */
/* 	    gasnet_AMRequestShort1(dest_node, MY_AM, payload ); */
/* 	  } else if (opt.rdma_write) { */
/* 	    gasnet_put_nbi_bulk( dest_node, dest, &payload, payload_size ); */
/* 	  } else if (opt.rdma_read) { */
/* 	    gasnet_get_nbi_bulk( &payload, dest_node, dest, payload_size ); */
/* 	  } */
/* 	} */
	GASNET_BEGIN_FUNCTION();
	if (opt.messages) {
	  for( j = base; j < base + frac; ++j ) {
	    //GASNET_BEGIN_FUNCTION();
	    //gasnet_AMRequestShort1(dest_node, MY_AM, payload);
            gasnet_AMRequestMedium0(dest_node, MY_AM2, source, opt.payload_size);
	  }
	} else if (opt.rdma_write) {
	  for( j = base; j < base + frac; ++j ) {
	    //GASNET_BEGIN_FUNCTION();
	    //gasnet_put_nbi_bulk( dest_node, dest, &payload, payload_size );
            //gasnet_put_nbi_bulk( dest_node, global_payload_p, global_payload_p, opt.payload_size );
            //gasnet_put( dest_node, global_payload_p, global_payload_p, opt.payload_size );
            gasnet_put_nbi_bulk( dest_node, dest, source, opt.payload_size );
	  }
	} else if (opt.rdma_read) {
	  for( j = base; j < base + frac; ++j ) {
	    //GASNET_BEGIN_FUNCTION();
	    //gasnet_get_nbi_bulk( &payload, dest_node, dest, payload_size );
            //gasnet_get_nbi_bulk( global_payload_p, dest_node, global_payload_p, opt.payload_size );
            gasnet_get( source, dest_node, dest, opt.payload_size );
	  }
	}
	//sent += frac;
      }
    gasnet_wait_syncnbi_all();
    

    sent = max;

    gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS);
    gasnet_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS);
    
    clock_gettime(CLOCK_MONOTONIC, &endtime);


    if ( gasnet_mynode() == gasnet_nodes() / 2 ) {
      uint64_t start_ns = ((uint64_t) starttime.tv_sec * 1000000000 + starttime.tv_nsec);
      uint64_t end_ns = ((uint64_t) endtime.tv_sec * 1000000000 + endtime.tv_nsec);
      double runtime = ( (double) end_ns - (double) start_ns ) / 1000000000.0;
      double message_rate = (double) opt.count / runtime;
      //double message_rate = (double) sent / runtime;
      double bandwidth = (double) opt.payload_size * message_rate;
      printf( "%d/%d messages in %f seconds = %f million messages per second = %f megabytes per second\n", sent, opt.count, runtime, message_rate / 1000 / 1000, bandwidth / 1024 / 1024 );


      char * conduit_start = strstr( GASNET_CONFIG_STRING, "CONDUIT=" );
      conduit_start += 8;
      char * conduit_end = strstr( conduit_start, "(" );
      char conduit[128] = { '\0' };
      strncpy( &conduit[0], conduit_start, conduit_end - conduit_start );

      char * message_type = opt.messages ? "active_messages" : opt.rdma_read ? "rdma_read" : opt.rdma_write ? "rdma_write" : "undefined";
      printf( "header, config, processes, sockets, cores, threads, count, sent, request_type, same_destination, runtime, message_rate, bandwidth\n" );
      printf( "data, %s, %d, %d, %d, %d, %d, %d, %s, %d, %f, %f, %f\n", conduit,
	      gasnet_nodes(), opt.sockets, opt.cores, opt.threads, opt.count, sent, message_type, opt.same_destination, runtime, message_rate / 1000 / 1000, bandwidth / 1024 / 1024 );
    }
    
    } else {
      
      gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS);
      gasnet_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS);
      
      ; // do nothing
      /* for( i = 0; i < max; ++i ) { */
      /* 	gasnet_AMPoll(); */
      /* } */
      
      gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS);
      gasnet_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS);
      
      printf("Node %d final value is %d.\n", gasnet_mynode(), *( (payload_t *) shared_memory_blocks[gasnet_mynode()].addr ) ); 
    }
    
    

    gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS);
    gasnet_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS);


    printf("Done.\n"); 
    gasnet_exit(0);
    return 0;
}
