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

int messages = 0;

void function_dispatch(int func_id, void *buffer, uint64 size) {
    ++messages;
    }
    
void ga_test() {
    struct global_array *ga;
    struct global_address addr;
    
    int i;
    
    ga = ga_allocate(sizeof(uint64), 10);
    
    if (gasnet_mynode() == 0) {
        for (i = 0; i < 10; i++) {
            ga_index(ga, i, &addr);
        
            printf("%d : node: %d address: %lld\n",
                i, addr.node, addr.offset);
            }
        }
        
    if (gasnet_mynode() == 1) {
        uint64  temp;
        
        for (i = 0; i < 10; i++) {
            ga_index(ga, i, &addr);
            temp = i;
            gm_copy_to(&temp, &addr, sizeof(uint64));            
            }
        }
    gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS);
    gasnet_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS);

    if (gasnet_mynode() == 0) {
        uint64  temp;
        
        for (i = 0; i < 10; i++) {
            ga_index(ga, i, &addr);
            temp = i;
            gm_copy_from(&addr, &temp, sizeof(uint64));
            printf("result: %d = %lld\n", i, temp);
            if (i != temp) {
                printf("Global array read/write test -- FAILED\n");
                exit(1);
                }
            }
        }
    gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS);
    gasnet_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS);
    printf("Global array read/write test -- SUCCESS\n");
    }

// Test remote allocations (ga_test will test local allocations).
void gm_test() {
    struct global_address addr;
    
    gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS);
    gasnet_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS);
    gm_allocate(&addr, 1 - gasnet_mynode(), sizeof(uint64));
    printf("%d allocation: %d at %lld\n", gasnet_mynode(), addr.node, addr.offset);
    gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS);
    gasnet_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS);
    }

void msg_test() {
    int i;
    struct global_address ga;
    
    ga.node = 1 - gasnet_mynode();
    ga.offset = 0;
    for (i = 0; i < 100; i++) {
        msg_send(&i, &ga, sizeof(int), 100);
        msg_send(&i, &ga, sizeof(int), 100);
        msg_flush_all();
        }
    }

void msg_test_complete() {
    if (messages != 200) {
        printf("Message Aggregator test -- FAILED\n");
        exit(1);
        }
    printf("Message Aggregaor test -- SUCCESS\n");
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
    gm_init();
    msg_aggregator_init();
    
    msg_test();
    gm_test();
    ga_test();
    msg_test_complete();
    
    gasnet_exit(0);
}
