#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sched.h>
#include <omp.h>
#include <queue>

#include <stdint.h>
// typical workaround for using c99 limits in c++ not working
#define UINT64_MAX 0xFFFFFFFFFFFFFFFF

#include <gasnet.h>
#include "getput.h"
#include "global_array.h"
#include "global_memory.h"
#include "msg_aggregator.h"

#include "timing.hpp"

#include "thread.h"

#include "CircularArray.hpp"
#include "CircularWSDeque.hpp"

#include "collective.h"

/*****DEBUG*************************************/
#define IS_DEBUG 0

#if IS_DEBUG
	#define DEBUGN(formatstr, ...) debug_print(formatstr, __VA_ARGS__)
#else
	#define DEBUGN(formatstr, ...) 0?0:0
#endif


void debug_print(const char* formatstr, ...) {
	va_list al;
	va_start(al, formatstr);
    vprintf(formatstr, al);
	va_end(al);
}

/**********************************************/

void __sched__noop__(pid_t, unsigned int, cpu_set_t*) {}
#define PIN_THREADS 1
#if PIN_THREADS
    #define SCHED_SET sched_setaffinity
#else
    #define SCHED_SET __sched__noop__
#endif

#define LOCAL_STEAL_RETRIES 1
#define REMOTE_STEAL_MAX_VICTIMS 6
#define REMOTE_STEAL_ABORT_RETRIES 1
#define MAX_STEALS_IN_FLIGHT 1

#define TERM_COUNT_WORK 2
#define TERM_COUNT_LOCAL_STEAL 2
#define TERM_COUNT_REMOTE_STEAL 6
#define TERM_COUNT_FACTOR 30

#define WORKSTEAL_REQUEST_HANDLER 188
#define WORKSTEAL_REPLY_HANDLER 187


#define STEAL_GLOBAL_NULL_SATISFIES 0


#define WAIT_BARRIER gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS); \
                     gasnet_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS)


void workStealRequestHandler(gasnet_token_t token, gasnet_handlerarg_t a0);
void workStealReplyHandler(gasnet_token_t token, gasnet_handlerarg_t a0, gasnet_handlerarg_t a1);

/*********************
 * gasnet process code
 **********************/

// This crys out for linker sets
gasnet_handlerentry_t   handlers[] =
    {
        { GM_REQUEST_HANDLER, (void (*) ()) gm_request_handler },
        { GM_RESPONSE_HANDLER, (void (*) ()) gm_response_handler },
        { GA_HANDLER, (void (*) ()) ga_handler },
        { WORKSTEAL_REQUEST_HANDLER, (void (*) ()) workStealRequestHandler },
        { WORKSTEAL_REPLY_HANDLER, (void (*) ()) workStealReplyHandler },
        { COLLECTIVE_REDUCTION_HANDLER, (void (*)()) serialReduceRequestHandler },
        { COLLECTIVE_RING_REDUCTION_HANDLER, (void (*)()) ringReduceRequestHandler }
    };

gasnet_seginfo_t* shared_memory_blocks;

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
/*************************/

typedef uint64_t TreeNode_ptr; //(global_array index)

#define MAX_NUM_CORES 6

CircularWSDeque<TreeNode_ptr> local_wsqs [MAX_NUM_CORES];

void spawnET(TreeNode_ptr t, uint64_t qid) {
    local_wsqs[qid].pushBottom(t);
}

#define TREENODE_NULL UINT64_MAX
#define TREENODE_FAIL UINT64_MAX - 1

std::queue<TreeNode_ptr> node_global_queue;
gasnet_hsl_t ngq_lock = GASNET_HSL_INITIALIZER; 


/**Tree Node**/
typedef struct TreeNode {
    TreeNode_ptr left;
    TreeNode_ptr right;
} TreeNode;

class IBT {

    int size, part;
    global_array* ibt;
    static const int LARGEPRIME = 961748941;
    
    public:
        int actualsize;
        IBT(int s, int f, int rank) : size(s), part(f) {
            if (part <= 0) part = 2;
            if (size >= LARGEPRIME)
                fprintf(stderr, "class IBT: size too big.\n"), exit(0);
             
             
            ibt = ga_allocate(sizeof(TreeNode), size);
            
            actualsize = 0;
            
            if (rank == 0) {
                BuildIBT(0, size);
            }
        }

        ~IBT() {delete ibt;}

//        void Print() {
//            PrintTree(ibt, 0);
//        }

        void Explore() {
            assert(false);
          //  ExploreTree(0); //root = index 0
        }

        global_array* getTreeArray() {
            return ibt;
        }

    private:

        /* Maps 0.. size-1 into itself, for the purpose of eliminating locality. */
        /* return i if a tree respecting spatial locality is desired. */
        int Permute(int i) {
            long long int j = i;
            return (j*LARGEPRIME) % size; 
            /*return i;*/
        }
        void BuildIBT(int root, int size) {
            int t1_size = (size-1)/part;
            int t2_size = size - t1_size - 1;
            TreeNode_ptr r = Permute(root); 
            actualsize++;
            TreeNode_ptr t1 = TREENODE_NULL;
            TreeNode_ptr t2 = TREENODE_NULL;
            if (t1_size) {
                BuildIBT(root+1, t1_size);
                t1 = Permute(root+1);
            }
            if (t2_size) {
                BuildIBT(root+1+t1_size, t2_size);
                t2 = Permute(root+1+t1_size);
            }
            if (random() % 2) {
                //r -> left = t1, r -> right = t2;
                TreeNode pd;
                pd.left = t1;
                pd.right = t2;
                put_remote(ibt, r, &pd, 0, sizeof(TreeNode));
            } else {
                //r -> left = t2, r -> right = t1;
                TreeNode pd;
                pd.left = t2;
                pd.right = t1;
                put_remote(ibt, r, &pd, 0, sizeof(TreeNode));
            }
        }

//        void PrintTree(TreeNode_ptr root, int depth) {
//            if (root) {
//                PrintTree(root -> right, depth + 1);
//                for (int i = 0; i < depth; i++) printf(" "); printf("O\n");
//                PrintTree(root -> left, depth + 1);
//            }
//        }

        
};


void ExploreTree(global_array* ibt, TreeNode_ptr root, uint64_t explorer_id, thread* me) {
    if (root!=TREENODE_NULL) {
        TreeNode rn;
        mem_tag_t tag = get_doublelong_nb(ibt, root, &rn);
        thread_yield(me);
        if (tag.dest) {
            rn = *((TreeNode*)tag.src);
        } else {
            gasnet_handle_t h = (gasnet_handle_t) tag.src;
            while (GASNET_OK != gasnet_try_syncnb(h)) {
                thread_yield(me);
            }
        }
        DEBUGN("tree %lu produced ", root);
        if (rn.left==TREENODE_NULL) DEBUGN("left:NULL, ");
        else DEBUGN("left:%lu, ", rn.left);
        if (rn.right==TREENODE_NULL) DEBUGN("right:NULL\n");
        else DEBUGN("right:%lu\n", rn.right);
           
        spawnET(rn.left, explorer_id );
        spawnET(rn.right, explorer_id );
    }
}

/*************/

// node global vars (used in AM handlers)
uint64_t num_cores = 1;
volatile int stealsInFlightCounts[MAX_NUM_CORES];

// global steal counts
volatile uint64_t numSuccessfulGlobalSteals[MAX_NUM_CORES];
volatile uint64_t numFailedGlobalSteals[MAX_NUM_CORES];
volatile uint64_t numNULLGlobalSteals[MAX_NUM_CORES];

typedef struct counts_t {
    uint64_t workCount;
    uint64_t termCount;
    uint64_t noWork;
} counts_t;

void swap(int* arr, int i, int j) {
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
}

void pushNodeGlobalQueue(TreeNode_ptr t) {
    gasnet_hsl_lock(&ngq_lock);
    node_global_queue.push(t);
    gasnet_hsl_unlock(&ngq_lock);
}

bool popNodeGlobalQueue(TreeNode_ptr* buf) {
    bool succeed = false;
    gasnet_hsl_lock(&ngq_lock);
    succeed = !node_global_queue.empty();
    if (succeed) {
        *buf = node_global_queue.front();
        node_global_queue.pop();
    }
    gasnet_hsl_unlock(&ngq_lock);

    return succeed;
}


typedef struct worker_info {
    int core_index;
    global_array* ibt;
    int myNode;
    int num_cores_per_node;
    int num_nodes;
    counts_t* counts;
    uint64_t terminationCount;
} worker_info;

void run(thread* me, int core_index, global_array* ibt, int myNode, int num_cores_per_node, int num_nodes, counts_t* counts, const uint64_t terminationCount); 

void thread_runnable(thread* me, void* arg) {
    struct worker_info* info = (struct worker_info*) arg;
    
    run( me,
        info->core_index,
         info->ibt,
         info->myNode,
         info->num_cores_per_node,
         info->num_nodes,
         info->counts,
         info->terminationCount );

    thread_exit(me, NULL);
}

void run(thread* me, int core_index, global_array* ibt, int myNode, const int num_cores_per_node, const int num_nodes, counts_t* counts, const uint64_t terminationCount) { 
    // 'counts' shared by threads on a core

    while (counts->termCount < terminationCount) {
        gasnet_AMPoll();

        bool gotWork = false;
        TreeNode_ptr work;

        DEBUGN("p%d c%d(t%d) -- (outer) visited:%lu\ttermC:%lu\n", myNode, core_index, me->id, counts->workCount, counts->termCount);

        // my work queue
        if (CWSD_OK == local_wsqs[core_index].popBottom(&work)) {
            DEBUGN("p%d c%d(t%d) -- myworkq\n", myNode, core_index, me->id);
            gotWork = true;
        } else {
            DEBUGN("p%d c%d(t%d) -- otherswq\n", myNode, core_index, me->id);
            while (!gotWork && counts->termCount < terminationCount) {
                DEBUGN("p%d c%d(t%d) -- (others) visited:%lu\ttermC:%lu\n", myNode, core_index, me->id, counts->workCount, counts->termCount);
                // steal locally
                int perm[num_cores_per_node];
                for (int i=0; i<num_cores_per_node; i++) {
                    perm[i] = i;
                }

                // exlcude self for local stealing
                swap(perm, core_index, 0);
            
                // try to steal locally in the random order
                // TODO: should a remote steal be started first if none in progress?
                for (int i=1; i<num_cores_per_node; i++) {
                    swap(perm, i, random()%(num_cores_per_node-i)+i);

                    int victim_queue = perm[i];
                    
                    counts->termCount+=TERM_COUNT_LOCAL_STEAL;
                    if (CWSD_OK == local_wsqs[victim_queue].steal(&work)) {
                        gotWork = true;
                        break;
                    }
                }

                if (num_nodes == 1) { thread_yield(me); continue; }

                if (!gotWork) {
                    gasnet_AMPoll();

                    // check node-global queue
                    if (popNodeGlobalQueue(&work)) {
                        gotWork = true;
                    }
                }

                if (!gotWork) { // TODO who shares remote steals? perhaps threads in a core?

                    // steal remote asynch
                    uint64_t oldSIF = __sync_fetch_and_add(&stealsInFlightCounts[core_index], 1);
                    if (oldSIF < MAX_STEALS_IN_FLIGHT) {
                        int victim_node = random()%num_nodes;
                        while (victim_node==myNode) victim_node = random()%num_nodes; // XXX: use permutation style instead

                        DEBUGN("p%d: sending steal request to p%d queue%d\n", myNode, victim_node, i % num_cores_per_node);
                        counts->termCount+=TERM_COUNT_REMOTE_STEAL;
                        DEBUGN("p%d c%d(t%d) -- fires remote steal to node%d\n", myNode, core_index, me->id, victim_node);
                        gasnet_AMRequestShort1 (victim_node, WORKSTEAL_REQUEST_HANDLER, core_index);
                    } else { 
                        // un-add since no steal started
                        __sync_fetch_and_sub (&stealsInFlightCounts[core_index], 1);
                    }
                    thread_yield(me);
                }
            }
        }

        /* do the work */
        if (gotWork) {  // might have terminated
            if (work!=TREENODE_NULL) {
                DEBUGN("p%d c%d(t%d) -- does work on TreeNode_ptr(%lu)\n", myNode, core_index, me->id, work);
                counts->workCount++;
            } else {  
                DEBUGN("p%d c%d(t%d) -- does work on TreeNode_ptr(NULL)\n", myNode, core_index, me->id);
            }

            counts->termCount+=TERM_COUNT_WORK;
            ExploreTree(ibt, work, core_index, me);
        }
    }
}

/*
void terminationRequestHandler(gasnet_token_t token, gasnet_handlerarg_t a0) {
    gasnet_hsl_lock(&term_lock);
    do_termination = true;
    gasnet_hsl_unlock(&term_lock);
}
*/



        
void process_cl_args(int* argc, char** argv[], int* size, int* imbalance, uint64_t* num_cores, uint64_t* num_threads_per_core);


int main(int argc, char* argv[]) {
    init(&argc, &argv);
   
    int size = 128;
    int imbalance = 2;
    uint64_t num_threads_per_core = 1;
    process_cl_args(&argc, &argv, &size, &imbalance, &num_cores, &num_threads_per_core);

    int rank = gasnet_mynode();
    int num_nodes = gasnet_nodes();

    WAIT_BARRIER;

    srandom(time(NULL));
    IBT* t = new IBT(size, imbalance, rank);
    
    WAIT_BARRIER;
    
    printf("actualsize:%d\n", t->actualsize);
    global_array* ibt = t->getTreeArray();
    // t->Print();


    // put root of tree in first work queue of the node that owns the root
    TreeNode_ptr root = 0;
    if (isLocal(ibt, root)) {
        local_wsqs[0].pushBottom(root);
    }

    // equal time per core, visit fraction of total, normalized by smallest time
    uint64_t per_core_term_count = (size / (num_nodes * num_cores)) * TERM_COUNT_FACTOR * TERM_COUNT_WORK;
    printf("per_core_term_count=%lu\n", per_core_term_count);
    counts_t counts[num_cores];
    

    // green threads init
    thread* masters[num_cores];
    scheduler* schedulers[num_cores]; 
    worker_info wis[num_cores][num_threads_per_core];

    #pragma omp parallel for num_threads(num_cores)
    for (uint64_t i = 0; i<num_cores; i++) {
        #pragma omp critical (crit_init)
        {
            // create master thread and scheduler 
            thread* master = thread_init();
            scheduler* sched = create_scheduler(master);
            masters[i] = master;
            schedulers[i] = sched;
        }

        counts[i].termCount = 0;
        counts[i].workCount = 0;
        counts[i].noWork = 0;
        stealsInFlightCounts[i] = 0;
        numSuccessfulGlobalSteals[i] = 0;
        numFailedGlobalSteals[i] = 0;
        numNULLGlobalSteals[i] = 0;
        for (uint64_t th=0; th<num_threads_per_core; th++) {
            wis[i][th].core_index = i;
            wis[i][th].ibt = ibt;
            wis[i][th].myNode = rank;
            wis[i][th].num_cores_per_node = num_cores;
            wis[i][th].num_nodes = num_nodes;
            wis[i][th].counts = &counts[i];
            wis[i][th].terminationCount = per_core_term_count;
            thread_spawn(masters[i], schedulers[i], thread_runnable, &wis[i][th]);
        }
    }


    Timer* timer = Timer::createSimpleTimer("total_time");
    printf("starting traversal\n");
    WAIT_BARRIER;
    timer->markTime(false);

    int coreslist[12] = {0,2,4,6,8,10,1,3,5,7,9,11};

    #pragma omp parallel for num_threads(num_cores)
    for (int core=0; core<num_cores; core++) {
        //TODO: this is 1 machine only pinning
        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(coreslist[rank*num_cores+core], &set);
        SCHED_SET(0, sizeof(cpu_set_t), &set);
        
        run_all(schedulers[core]);
        printf("p%d c%d quit [visited=%lu, termCount=%lu]\n", rank, core, counts[core].workCount, counts[core].termCount);
    }

    WAIT_BARRIER;
    timer->markTime(true);

    const double runtime = timer->avgIntervalNs();

    // combine core counts into node-level counts
    uint64_t my_work_count = 0;
    uint64_t my_nowork_count = 0;
    uint64_t my_failed_global_steal_count = 0;
    uint64_t my_successful_global_steal_count = 0;
    uint64_t my_NULL_global_steal_count = 0;
    for (uint64_t core=0; core<num_cores; core++) {
        my_work_count += counts[core].workCount;
        my_nowork_count += counts[core].noWork;
        my_failed_global_steal_count += numFailedGlobalSteals[core];
        my_successful_global_steal_count += numSuccessfulGlobalSteals[core];
        my_NULL_global_steal_count += numNULLGlobalSteals[core];
    }

    printf("node%d work=%lu, nowork=%lu\n", rank, my_work_count, my_nowork_count);

    printf("node%d failed global steals:%lu; null global steals:%lu, successful global steals:%lu; (%f)\n", rank, my_failed_global_steal_count, my_NULL_global_steal_count, my_successful_global_steal_count, (double)my_successful_global_steal_count/(my_successful_global_steal_count + my_failed_global_steal_count + my_NULL_global_steal_count));

    WAIT_BARRIER;
    //uint64_t global_work_count = serialReduce(COLL_ADD, 0, my_work_count);
    uint64_t global_work_count = ringReduce(COLL_ADD, 0, my_work_count);
    WAIT_BARRIER;

    if (0 ==rank) 
        printf("global_work_count:%lu\tsize:%lu\n", global_work_count, size);

    const double rate_M = (global_work_count / runtime) * 1000;

    if (0 == rank) 
        printf("%f Mref/s\n", rate_M);


    gasnet_exit(0);
    return 0;
}



void workStealRequestHandler(gasnet_token_t token, gasnet_handlerarg_t a0) {
    /**/
    gasnet_node_t fromNode;
    gasnet_AMGetMsgSource(token, &fromNode);
    DEBUGN("got remote steal request from %d\n", fromNode);
    /**/

    uint32_t coreid = (uint32_t) a0;

    int num_victim_tries = (REMOTE_STEAL_MAX_VICTIMS < num_cores) ? REMOTE_STEAL_MAX_VICTIMS : num_cores;

    int perm[num_cores];
    for (int i=0; i<num_cores; i++) {
        perm[i] = i;
    }

    bool gotWork = false;
    bool gotNullWork = false;
    TreeNode_ptr work;

    // steal in random order
    for (int i=0; (i<num_victim_tries) && !gotWork; i++) {
        swap(perm, i, random()%(num_cores-i)+i);

        int victim_queue = perm[i];

        bool done = false;
        int trynum = 0;
        while (!done && trynum <= REMOTE_STEAL_ABORT_RETRIES) {
            cwsd_status stat = local_wsqs[victim_queue].steal(&work);
            switch(stat) {
                case CWSD_ABORT:
                    DEBUGN(" p%d: (%lu,%d)ABORT\n", gasnet_mynode(), fromNode, token);
                    trynum++;
                    break;
                case CWSD_OK:
                    done = true;

                    #if STEAL_GLOBAL_NULL_SATISFIES
                        gotWork = true;
                    #else
                        if (work != TREENODE_NULL) {
                            gotWork = true;
                        } else {
                            gotNullWork = true;
                        }
                    #endif
                    
                    
                    DEBUGN(" p%d: (%lu,%d)OK node=%lu\n", gasnet_mynode(), fromNode, token, t);
                    break;
                default:
                    done = true;
            }
        }
    }

    #if STEAL_GLOBAL_NULL_SATISFIES
    if (!gotWork) {
    #else
    if (!gotWork && !gotNullWork) {
    #endif 
        work = TREENODE_FAIL; // communicate that the steal failed
        DEBUGN("steal request from(%d) gets NODE=FAIL\n", fromNode, work);
    #if STEAL_GLOBAL_NULL_SATISFIES
    } else if (work==TREENODE_NULL) {
    #else
    } else if (!gotWork) {
        work = TREENODE_NULL;
    #endif
        DEBUGN("steal request from(%d) gets NODE=NULL\n", fromNode, work);
    } else {
        DEBUGN("steal request from(%d) gets NODE=%lu\n", fromNode, work);
    }

    gasnet_AMReplyShort2 (token, WORKSTEAL_REPLY_HANDLER, work, coreid);
}


void workStealReplyHandler(gasnet_token_t token, gasnet_handlerarg_t a0, gasnet_handlerarg_t a1) {
    TreeNode_ptr t = (TreeNode_ptr) a0;
    uint32_t coreid = (uint32_t) a1;
    
    
    //if (TREENODE_NULL == t) printf("p%d: recv reply(%lu, %d) to core%d with TreeNode_ptr=TREENULL\n", gasnet_mynode(), token, fromNode, coreid);
    //else printf("p%d: recv reply(%lu, %d) to core%d with TreeNode_ptr=%lu\n", gasnet_mynode(), token, fromNode, coreid, t);
  
    __sync_fetch_and_sub(&stealsInFlightCounts[coreid], 1);

    switch (t) {
        case TREENODE_NULL:
            __sync_fetch_and_add(&numNULLGlobalSteals[coreid], 1);
            break;
        case TREENODE_FAIL:
            __sync_fetch_and_add(&numFailedGlobalSteals[coreid], 1);
            break;
        default:
            pushNodeGlobalQueue(a0);
            __sync_fetch_and_add(&numSuccessfulGlobalSteals[coreid], 1);
    }
}


void usage() {
    fprintf(stderr, "creates binary tree with <size> > 0 nodes each internal node having one subtree with 1/<imbalance> of the descendents.\nFor example\n\ttree -n 100 -i 2\ncreates a balanced binary tree with 100 nodes.\n\ttree -n 100 -i 3\ncreates a tree in which each subtree has about either half or twice the nodes of its sibling, ensuring moderate imbalance.\n");
}

void process_cl_args(int* argc, char** argv[], int* size, int* imbalance, uint64_t* num_cores, uint64_t* num_threads_per_core) {
    
    static struct option long_options[] = {
        {"size", required_argument, NULL, 'n'},
        {"imbalance", required_argument, NULL, 'i'},
        {"num_cores", required_argument, NULL, 'c'},
        {"num_threads_per_core", required_argument, NULL, 't'},
        {"help", no_argument, NULL, 'h'},
        {NULL, no_argument, NULL, 0}
    };
    int c, option_index=1;
    while ((c = getopt_long(*argc, *argv, "n:i:c:t:h?",
                    long_options, &option_index)) >= 0) {
        switch (c) {
            case 0:   // flag set
                break;
            case 'n':
                *size = atoi(optarg);
                break;
            case 'i':
                *imbalance = atoi(optarg);
                break;
            case 'c':
                *num_cores = atoi(optarg);
                break;
            case 't':
                *num_threads_per_core = atoi(optarg);
                break;
            case 'h':
            case '?':
            default:
                if (0 == gasnet_mynode()) {
                    usage();
                    printf("\nAvailable options:\n");
                    for (struct option* optp = &long_options[0]; optp->name != NULL; ++optp) {
                        if (optp->has_arg == no_argument) {
                            printf("  -%c, --%s\n", optp->val, optp->name);
                        } else {
                            printf("  -%c, --%s <ARG>\n", optp->val, optp->name);
                        }
                    }
                }
                exit(1);
        }
    }
}
