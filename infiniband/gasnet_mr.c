
#include <mpi.h>
#include <gasnet.h>
#include <stdint.h>
#include <assert.h>

#include "options.h"

//#include "base.h"
//#include "global_memory.h"
//#include "global_array.h"
//#include "msg_aggregator.h"

gasnet_seginfo_t    *shared_memory_blocks;

typedef char payload_t;
int payload_size;
payload_t payload;


#define MY_AM 129
void my_am(gasnet_token_t token,
	   gasnet_handlerarg_t a0) {
  //    uint64_t * dest = (uint64_t *) shared_memory_blocks[1].addr;
  payload_t * dest = (payload_t *) shared_memory_blocks[gasnet_mynode()].addr;
    *dest += a0;
}



// This crys out for linker sets
gasnet_handlerentry_t   handlers[] =
    {
      { MY_AM, (void *)my_am },
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

    struct options opt = parse_options( &argc, &argv );

    if (0) {
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

    if (0) {
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

    if (gasnet_mynode() >= gasnet_nodes() / 2) {

      int i;

      struct timespec starttime;
      struct timespec endtime;
      
      clock_gettime(CLOCK_MONOTONIC, &starttime);
      
      gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS);
      gasnet_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS);
      
      int dest_node = gasnet_mynode() - gasnet_nodes() / 2;
      if (opt.same_destination) {
	dest_node = 0;
      }

    payload_t * dest = (payload_t *) shared_memory_blocks[dest_node].addr;
    payload = 1;

#pragma omp parallel for num_threads( opt.threads )
    for( i = 0; i < opt.count / (gasnet_nodes() / 2); ++i ) 
      {
	if (opt.messages) {
	  gasnet_AMRequestShort1(dest_node, MY_AM, payload );
	} else if (opt.rdma_write) {
	  gasnet_put_nbi_bulk( dest_node, dest, &payload, payload_size );
	} else if (opt.rdma_read) {
	  gasnet_get_nbi_bulk( &payload, dest_node, dest, payload_size );
	}
      }
    gasnet_wait_syncnbi_all();
    
    
    gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS);
    gasnet_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS);
    
    clock_gettime(CLOCK_MONOTONIC, &endtime);
    
    if ( gasnet_mynode() == gasnet_nodes() / 2 ) {
      uint64_t start_ns = ((uint64_t) starttime.tv_sec * 1000000000 + starttime.tv_nsec);
      uint64_t end_ns = ((uint64_t) endtime.tv_sec * 1000000000 + endtime.tv_nsec);
      double runtime = ( (double) end_ns - (double) start_ns ) / 1000000000.0;
      double message_rate = (double) opt.count / runtime;
      double bandwidth = (double) payload_size * message_rate;
      printf( "%d messages in %f seconds = %f million messages per second = %f megabytes per second\n", opt.count, runtime, message_rate / 1000 / 1000, bandwidth / 1024 / 1024 );
      char * message_type = opt.messages ? "active_messages" : opt.rdma_read ? "rdma_read" : opt.rdma_write ? "rdma_write" : "undefined";
      printf( "header, processes, sockets, cores, threads, count, request_type, same_destination, runtime, message_rate, bandwidth\n" );
      printf( "data, %d, %d, %d, %d, %d, %s, %d, %f, %f, %f\n", 
	      gasnet_nodes(), opt.sockets, opt.cores, opt.threads, opt.count, message_type, opt.same_destination, runtime, message_rate / 1000 / 1000, bandwidth / 1024 / 1024 );
    }
    
    } else {
      
      gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS);
      gasnet_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS);
      
      ; // do nothing
      
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
