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

static int finished = 0;
#define FINISHED 100

void function_dispatch(int func_id, void *buffer, uint64 size) {
    switch (func_id) {
        case    FINISHED:
            ++finished;
        }
    }
    
static void init(int *argc, char ***argv) {
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

static void usage() {
    if (gasnet_mynode() == 0)
        printf( "bwtest [-array_size <elements>] [-element_size <bytes>]"
            " [-requests_per_node <#>] [-outstanding_requests <#>\n");
    gasnet_exit(1);
}

void idle_loop() {
    msg_flush_all();
    gasnet_AMPoll();
    address_expose();
}

static void perf_test(int argc, char **argv) {
    uint64  array_size = 64;
    uint64  element_size = 8;
    uint64  requests = 10000;
    uint64  outstanding_requests = 100;
    uint64  start_time, end_time;
    int i;
    struct global_array *ga;
    uint64 index, index2, index3;
    struct global_address *blocks, addr;
    volatile uint8 *bptr;
    uint64 start, end;
    struct msg_pull_request *pr;
    
    srand(gasnet_mynode());
    
    //// Process arguments
    i = 1;
    while (i < argc) {
        if (!argv[i][0] == '-')
            usage();

        switch (argv[i][1]) {
#define UINT64_ARG(l, u, v) \
            case    l:  \
            case    u:  \
                if ((i + 1) >= argc)    \
                    usage();    \
                v = atoi(argv[i + 1]);  \
                i += 2; \
                break;
            
            UINT64_ARG('a', 'A', array_size);
            UINT64_ARG('e', 'E', element_size);
            UINT64_ARG('r', 'R', requests);
            UINT64_ARG('o', 'O', outstanding_requests);
            
            case    '?':
            case    'H':
            case    'h':
                usage();
                
            default:
                printf("Cannot understand argument: %s\n", argv[i]);
                usage();
            }
        }
    if (array_size < gasnet_nodes()) {
        printf("array_size should be >= number of nodes (%lld is < %d)\n",
            array_size, gasnet_nodes());
        usage();
        }
    if (outstanding_requests > requests) {
        printf( "outstanding_requests > requests_per_node,"
                " should be <= (%lld > %lld)\n",
            outstanding_requests, requests);
        usage();
        }
        
    printf( "configuration: array_size = %lld element_size = %lld"
            " requests_per_node = %lld outstanding_requests = %lld\n",
        array_size, element_size, requests, outstanding_requests);

    printf("Initializing memory\n");
    fflush(stdout);


    // Allocate request blocks
    blocks = malloc(sizeof(struct global_address) * outstanding_requests);
    for (index = 0; index < outstanding_requests; index++) {
        gm_allocate(&blocks[index], gasnet_mynode(), element_size);
        bptr = gm_local_gm_address_to_local_ptr(&blocks[index]);
        for (index2 = 0; index2 < element_size; index2++)
            bptr[index2] = 1;
        }
    // Allocate global array    
    ga = ga_allocate(element_size, array_size);
    
    // Fill array with all 1 bytes
    ga_local_range(ga, &start, &end);
    for (index = start; index <= end; index++) {
        ga_index(ga, index, &addr);
        bptr = gm_local_gm_address_to_local_ptr(&addr);
        for (index2 = 0; index2 < element_size; index2++)
            bptr[index2] = 1;
        }
    printf("Performing raw message test ....\n");
    fflush(stdout);

    start_time = rdtsc();
    index = index2 = 0;
    while (index < requests) {
        // ensure the block is free
        bptr = gm_local_gm_address_to_local_ptr(&blocks[index2]);
        while (*bptr == 0) {
            gasnet_AMPoll();
            address_expose();
            }
        *bptr = 0;
        // copy a random block from a remote node
        index3 = rand() % array_size;
        ga_index(ga, index3, &addr);
        gm_copy_from(&addr, bptr, element_size);
        index2 = (index2 + 1) % outstanding_requests;
        ++index;
        }
    index = 2;
    while (index2 < outstanding_requests) {
        // ensure the block is free
        bptr = gm_local_gm_address_to_local_ptr(&blocks[index2]);
        while (*bptr == 0 && index2 < requests) {
            gasnet_AMPoll();
            address_expose();
            }
        ++index2;
        }
    end_time = rdtsc();
    printf("Time on node %d: %lldMcycles or %lld cycles/request\n",
        gasnet_mynode(),
        (end_time - start_time) / ONE_MILLION,
        (end_time - start_time) / requests);
        
    fflush(stdout);
    gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS);
    gasnet_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS);

    pr = malloc(sizeof(struct msg_pull_request) * outstanding_requests);
    for (index = 0; index < outstanding_requests; index++) {
        bptr = gm_local_gm_address_to_local_ptr(&blocks[index]);
        for (index2 = 0; index2 < element_size; index2++)
            bptr[index2] = 1;
        }

    printf("Performing aggregator message test ....\n");
    fflush(stdout);

    start_time = rdtsc();
    index = index2 = 0;
    while (index < requests) {
        // ensure the block is free
        bptr = gm_local_gm_address_to_local_ptr(&blocks[index2]);
        while (*bptr == 0)
            idle_loop();

        *bptr = 0;
        // copy a random block from a remote node
        index3 = rand() % array_size;
        ga_index(ga, index3, &addr);
        
        pr[index2].to_address = blocks[index2];
        pr[index2].size = element_size;
        
        msg_copy_from(&addr, &pr[index2]);
        
        index2 = (index2 + 1) % outstanding_requests;
        ++index;
        }
    index = 2;
    while (index2 < outstanding_requests) {
        // ensure the block is free
        bptr = gm_local_gm_address_to_local_ptr(&blocks[index2]);
        while (*bptr == 0)
            idle_loop();
        ++index2;
        }
    end_time = rdtsc();
    printf("Time on node %d: %lldMcycles or %lld cycles/request\n",
        gasnet_mynode(),
        (end_time - start_time) / ONE_MILLION,
        (end_time - start_time) / requests);
    fflush(stdout);

    // tell everyone we are finished
    for(i = 0; i < gasnet_nodes(); i++) {
        addr.node = i;
        msg_send(&i, &addr, sizeof(int), FINISHED);
        }
    while (finished < gasnet_nodes())
        idle_loop();
    printf("Finished\n");
    gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS);
    gasnet_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS);
    
}
 
int main(int argc, char **argv) {
    init(&argc, &argv);
    perf_test(argc, argv);
    gasnet_exit(0);
}
