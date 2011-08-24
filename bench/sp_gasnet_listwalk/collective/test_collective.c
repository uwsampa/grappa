#include <mpi.h>
#include <gasnet.h>
#include "base.h"
#include "global_memory.h"
#include "global_array.h"
#include "msg_aggregator.h"

#include "collective.h"

// This crys out for linker sets
gasnet_handlerentry_t   handlers[] =
    {
        { GM_REQUEST_HANDLER, (void *)gm_request_handler },
        { GM_RESPONSE_HANDLER, (void *)gm_response_handler },
        { GA_HANDLER, (void *)ga_handler },
        { COLLECTIVE_REDUCTION_HANDLER, (void *) serialReduceRequestHandler }
    };

gasnet_seginfo_t    *shared_memory_blocks;

#define SHARED_PROCESS_MEMORY_SIZE  (ONE_MEGA * 256)
#define SHARED_PROCESS_MEMORY_OFFSET (ONE_MEGA * 256)

void function_dispatch(int func_id, void *buffer, uint64_t size) {
    }
    
void init(int *argc, char ***argv) {
    int initialized = 0;
    
    MPI_Initialized(&initialized);
    if (!initialized)
        if (MPI_Init(argc, argv) != MPI_SUCCESS) {
        printf("Failed to initialize MPI\n");
        exit(1);
    }
        
    if (gasnet_init(argc, argv) != GASNET_OK) {
        printf("Failed to initialize gasnet\n");
        exit(1);
    }
    
    if (gasnet_attach(handlers,
        sizeof(handlers) / sizeof(gasnet_handlerentry_t),
        SHARED_PROCESS_MEMORY_SIZE,
        SHARED_PROCESS_MEMORY_OFFSET) != GASNET_OK) {
        printf("Failed to allocate sufficient shared memory with gasnet\n");
        gasnet_exit(1);
    }
    gm_init();

    printf("using GASNET_SEGMENT_FAST=%d; maxGlobalSegmentSize=%lu\n", GASNET_SEGMENT_FAST, gasnet_getMaxGlobalSegmentSize());
}
    
int main(int argc, char **argv) {
    init(&argc, &argv);

    int rank = gasnet_mynode();
    uint64_t myValue = (rank+1)*(rank+1);
    
    if (rank==0) printf("Starting add reduction\n");
    uint64_t addResult = serialReduce(COLL_ADD, 0, myValue);
    
    gasnet_barrier_notify(11, 0);
    gasnet_barrier_wait(11, 0);

    if (rank==0) printf("Starting max reduction\n");
    uint64_t maxResult = serialReduce(COLL_MAX, 0, rank);

    if (rank==0) {
        printf("addResult = %lu, maxResult=%lu for %d nodes\n", addResult, maxResult, gasnet_nodes());
    }
    
    gasnet_exit(0);
}
