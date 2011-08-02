#include <mpi.h>
#include <gasnet.h>
#include "base.h"
#include "global_memory.h"
#include "global_array.h"
#include "msg_aggregator.h"

// This crys out for linker sets
gasnet_handlerentry_t   handlers[] =
    {
        { GM_REQUEST_HANDLER, (void *)gm_request_handler },
        { GM_RESPONSE_HANDLER, (void *)gm_response_handler },
        { GA_HANDLER, (void *)ga_handler },
        { MSG_AGGREGATOR_DISPATCH, (void *)msg_aggregator_dispatch_handler },
        { MSG_AGGREGATOR_REPLY, (void *)msg_aggregator_reply_handler }
    };

gasnet_seginfo_t    *shared_memory_blocks;

#define SHARED_PROCESS_MEMORY_SIZE  (ONE_MEGA * 256)
#define SHARED_PROCESS_MEMORY_OFFSET (ONE_MEGA * 256)

void function_dispatch(int func_id, void *buffer, uint64 size) {
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
    msg_aggregator_init();
}
    
int main(int argc, char **argv) {
    init(&argc, &argv);
    
    gasnet_exit(0);
}
