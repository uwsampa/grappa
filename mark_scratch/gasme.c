#include <mpi.h>
#include <gasnet.h>
#include "base.h"
#include "global_memory.h"

void test_handler(
    gasnet_token_t  token,
    gasnet_handlerarg_t arg0) {
    printf("test_handler on %d: sizeof(t)=%ld %ld\n",
        gasnet_mynode(),
        sizeof(gasnet_handlerarg_t),
        (long int)arg0);
}

gasnet_handlerentry_t   handlers[] =
    {
        { 128, (void *)test_handler },
        { GM_REQUEST_HANDLER, (void *)gm_request_handler },
        { GM_RESPONSE_HANDLER, (void *)gm_response_handler }
    };

gasnet_seginfo_t    *shared_memory_blocks;

#define SHARED_PROCESS_MEMORY_SIZE  (ONE_MEGA * 256)
#define SHARED_PROCESS_MEMORY_OFFSET (ONE_MEGA * 256)

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

    if (gasnet_mynode() == 0) {
        for (i = 0; i < gasnet_nodes(); i++) {
            printf("SHM on %d: at %p of %llx bytes\n",
                i,
                shared_memory_blocks[i].addr,
                (unsigned long long) shared_memory_blocks[i].size);
            }
        }
        
    printf("Rocken\n");
    {
    
    
    
    for(i = 0; i < gasnet_nodes(); i++)
        gasnet_AMRequestShort1(1 - gasnet_mynode(), 128, gasnet_mynode());
    }
    
    gasnet_AMPoll();    
    gasnet_exit(0);
}
