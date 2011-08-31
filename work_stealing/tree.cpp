#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

#include <stdint.h>

// typical workaround for using c99 limits in c++ not working
#define UINT64_MAX 0xFFFFFFFFFFFFFFFF

#include <gasnet.h>
#include "getput.h"
#include "global_array.h"
#include "global_memory.h"
#include "msg_aggregator.h"
#include "tasklib.h"

#include "timing.hpp"

#include "CircularArray.hpp"
#include "CircularWSDeque.hpp"

#ifdef MTA
#define myrandom MTA_CLOCK(0)
#else
#define myrandom random()
#define future
#endif

/* Imbalanced Binary Tree class
 * ** An imbalanced binary tree is constructed such that sibling subtrees 
 * ** are of substantially different size.  In this implementation, a tree of any
 * ** specified "size" nodes is constructed with each subtree having one child
 * ** with a specified fraction 1/"part" of that subtree's nodes while its sibling
 * ** has the remainder.  With increasing value of "part", the tree produced
 * ** has increasing imbalance.
 * ** As written, the tree is constructed to exhibit little spatial locality.
 * ** By replacing Permute with the identity function, a tree that respects locality
 * ** is constructed, in the sense that every subtree fills contiguous locations.
 * */

#define WORKSTEAL_REQUEST_HANDLER 188
#define WORKSTEAL_REPLY_HANDLER 187

#define WAIT_BARRIER gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS); \
                     gasnet_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS)


void workStealRequestHandler(gasnet_token_t token, gasnet_handlerarg_t a0, gasnet_handlerarg_t a1, gasnet_handlerarg_t a2);
void workStealReplyHandler(gasnet_token_t token, gasnet_handlerarg_t a0, gasnet_handlerarg_t a1, gasnet_handlerarg_t a2);

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
        { WORKSTEAL_REPLY_HANDLER, (void (*) ()) workStealReplyHandler }
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
#define MAX_NUM_THREADS_PER_CORE 64
gasnet_hsl_t steal_locks[MAX_NUM_CORES];
TreeNode_ptr steal_boxes[MAX_NUM_CORES][MAX_NUM_THREADS_PER_CORE];
bool steal_full[MAX_NUM_CORES][MAX_NUM_THREADS_PER_CORE];

CircularWSDeque<TreeNode_ptr> local_wsqs [MAX_NUM_CORES];

#define TREENODE_NULL UINT64_MAX

int global_print_count=0;
void printCount() {
   int old =  __sync_fetch_and_add(&global_print_count, 1);
   printf("p%d: %d node visited\n", gasnet_mynode(), old);
}



typedef struct TreeNode {
    TreeNode_ptr left;
    TreeNode_ptr right;
} TreeNode;

class IBT {

    int size, part;
    global_array* ibt;
    static const int LARGEPRIME = 961748941;
    
    public:
        IBT(int s, int f) : size(s), part(f) {
            if (part <= 0) part = 2;
            if (size >= LARGEPRIME)
                fprintf(stderr, "class IBT: size too big.\n"), exit(0);
             
             
            ibt = ga_allocate(sizeof(TreeNode), size);
            
            
            BuildIBT(0, size);
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
            if (myrandom % 2) {
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

        /*MACROIFIED for yield
        void ExploreTree(TreeNode_ptr root) {
            if (root) {
                TreeNode rn;
                mem_tag_t tag = get_double_long_nb(ibt, root, &rn);
                yield();
                if (tag.dest) {
                    rn = *((TreeNode*)tag.src);
                } else {
                    gasnet_handle_t h = (gasnet_handle_t) tag.src;
                    while (GASNET_OK != gasnet_try_syncnb(h)) {
                        yield();
                    }
                }
                spawnET(rn.left);
                spawnET(rn.right);
            }
        }
        */
};

template class CircularArray<TreeNode_ptr>;
template class CircularWSDeque<TreeNode_ptr>;


typedef struct worker_data {
    int index; 
    bool gotWork;
    TreeNode_ptr w;
    TreeNode wnode;
    mem_tag_t tag;
} worker_data;


void spawnET(TreeNode_ptr t, CircularWSDeque<TreeNode_ptr>* my_wsq) {
    my_wsq->pushBottom(t);
}

void run(int core_index, int num_threads, global_array* ibt, int myNode, int num_nodes) {
    task threads[num_threads];
    worker_data datas[num_threads];
    
    task_scheduler(threads, datas, num_threads, worker_data) {
        current_data->index = current_scheduler->current_task;
        gasnet_hsl_t* lock = &steal_locks[core_index];
        while (1) {

            current_data->gotWork = false; 
            /** get work **/
            {
    
            // get work "body"
            if (CWSD_OK == local_wsqs[core_index].popBottom(&current_data->w)) {
                current_data->gotWork = true;  /* return */
            } else { // try to steal
                int i=core_index;
                while(i==core_index) i=random()%num_threads; //XXX num threads

                int victim_node = i / num_nodes;
                if (victim_node==myNode) {             // TODO hierarchical heuristic to try stealing local first
                    // steal from a local queue
                    if (CWSD_OK == local_wsqs[i].steal(&current_data->w)) {
                        current_data->gotWork = true; /* return */
                    } else {
                        current_data->gotWork = false; /* return */
                    }
                } else {
                    gasnet_hsl_lock(lock); // TODO: volatile and compiler mem barrier
                    steal_full[core_index][current_data->index] = false;
                    gasnet_hsl_unlock(lock);
                    
                    printf("p%d: sending steal request to p%d\n", myNode, victim_node);
                    gasnet_AMRequestShort3 (victim_node, WORKSTEAL_REQUEST_HANDLER, i % num_nodes, core_index, current_data->index);
                    yield();
                    while (true) {
                        gasnet_hsl_lock(lock);
                        if (steal_full[core_index][current_data->index]) {
                            current_data->w = steal_boxes[core_index][current_data->index];
                            gasnet_hsl_unlock(lock);

                            if (TREENODE_NULL==current_data->w) { // null indicates failure to steal
                                current_data->gotWork = false; /* return */
                                break;
                            }

                            current_data->gotWork = true; /* return */
                            break;
                        } else {
                            // wait for reply
                            gasnet_hsl_unlock(lock);
                            gasnet_AMPoll();
                            yield();
                        }
                    }
                }
            }

            }
            /** end get work **/

            if (current_data->gotWork) {
                /*ExploreTree(w, ibt);*/
                if (current_data->w!=TREENODE_NULL) { 
                    printCount(); 
                    current_data->tag = get_doublelong_nb(ibt, current_data->w, &current_data->wnode); 
                    yield(); 
                    if (current_data->tag.dest) { 
                        current_data->wnode = *((TreeNode*)current_data->tag.src); 
                    } else { 
                        while (GASNET_OK != gasnet_try_syncnb(  (gasnet_handle_t) current_data->tag.src)  ) { 
                            yield(); 
                        } 
                    } 
                    spawnET(current_data->wnode.left, &local_wsqs[core_index]); 
                    spawnET(current_data->wnode.right, &local_wsqs[core_index]); 
                } 

            } else {
                yield();
                continue; // TODO figure our termination
            }
        }
    } end_scheduler();
}




void process_cl_args(int* argc, char** argv[], int* size, int* imbalance, uint64_t* num_cores, uint64_t* num_threads_per_core);



int main(int argc, char* argv[]) {
    init(&argc, &argv);
   
    int size = 128;
    int imbalance = 2;
    uint64_t num_cores = 1;
    uint64_t num_threads_per_core = 1;
    process_cl_args(&argc, &argv, &size, &imbalance, &num_cores, &num_threads_per_core);

    for (int core=0; core<num_cores; core++) {
        gasnet_hsl_init(&steal_locks[core]);
    }

    WAIT_BARRIER;

    IBT* t = new IBT(size, imbalance);
    global_array* ibt = t->getTreeArray();
    // t->Print();

    int rank = gasnet_mynode();
    int num_nodes = gasnet_nodes();


    // put root of tree in first work queue of the node that owns the root
    TreeNode_ptr root = 0;
    if (isLocal(ibt, root)) {
        local_wsqs[0].pushBottom(root);
    }

    Timer* timer = Timer::createSimpleTimer("total_time");
    printf("starting traversal\n");
    WAIT_BARRIER;
    timer->markTime(false);

    #pragma omp parallel for num_threads(num_cores)
    for (int core=0; core<num_cores; core++) {
        run(core, num_threads_per_core, ibt, rank, num_nodes);
    }

    WAIT_BARRIER;
    timer->markTime(true);

    const double runtime = timer->avgIntervalNs();
    const double rate_M = (size / runtime) * 1000;

    if (0 == rank) 
        printf("%f Mref/s\n", rate_M);


    // cleanup
    for (int core=0; core<num_cores; core++) {
        gasnet_hsl_destroy(&steal_locks[core]);
    }

    gasnet_exit(0);
    return 0;
}

#define STEAL_ABORT_RETRIES 1
void workStealRequestHandler(gasnet_token_t token, gasnet_handlerarg_t a0, gasnet_handlerarg_t a1, gasnet_handlerarg_t a2) {
    uint64_t local_index = (uint64_t) a0;
    uint32_t coreid = (uint32_t) a1;
    uint32_t threadid = (uint32_t) a2;


    printf("remote core %d requests to steal from wsq%d\n", coreid, local_index);

    TreeNode_ptr t;
    
    //TODO if ABORT (instead of EMPTY), it may be worth retrying a bounded number of times
    int trynum = 0;
    bool done = false;
    while (!done && trynum <= STEAL_ABORT_RETRIES) {
        cwsd_status stat = local_wsqs[local_index].steal(&t);
        switch(stat) {
            case CWSD_ABORT:
                trynum++;
                break;
            case CWSD_EMPTY:
                t = TREENODE_NULL;
            default:
                done = true;
        }
    }

    gasnet_AMReplyShort3 (token, WORKSTEAL_REPLY_HANDLER, t, coreid, threadid);
}

void workStealReplyHandler(gasnet_token_t token, gasnet_handlerarg_t a0, gasnet_handlerarg_t a1, gasnet_handlerarg_t a2) {
    TreeNode_ptr t = (TreeNode_ptr) a0;
    uint32_t coreid = (uint32_t) a1;
    uint32_t threadid = (uint32_t) a2;
    
    if (TREENODE_NULL == t) printf("reply to core%d with TreeNode_ptr=TREENULL\n", coreid);
    else printf("reply to core%d with TreeNode_ptr=%lu\n", coreid, t);
   
    gasnet_hsl_lock(&steal_locks[coreid]);
    steal_boxes[coreid][threadid] = t;
    steal_full[coreid][threadid] = true;
    gasnet_hsl_unlock(&steal_locks[coreid]);
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


//bool getWork(CircularWSDeque<TreeNode_ptr>* qs, int myIndex, TreeNode_ptr* ele) {
//    TreeNode_ptr res;
//    if (CWSD_OK == qs[myIndex].popBottom(&res)) {
//        *ele = res;
//        return true;
//    } else { // try to steal
//        int i=myIndex;
//        while(i==myIndex) i=random()%NUM_THREADS; //XXX num threads
//        
//        int victim_node = i / num_nodes;
//        if (victim_node==myNode) {             // TODO hierarchical heuristic to try stealing local first
//            // steal from a local queue
//            if (CWSD_OK == qs[i].steal(&res)) {
//                *ele = res;
//                return true;
//            } else {
//                return false;
//            }
//        } else {
//            gasnet_hsl_t* lock = &steal_locks[myIndex];
//
//            gasnet_hsl_lock(lock); // TODO: volatile and compiler mem barrier
//            steal_full[myIndex] = false;
//            gasnet_hsl_unlock(lock);
//
//            gasnet_AMRequestShort2 (victim_node, WORKSTEAL_REQUEST_HANDLER, i % num_nodes, myIndex);
//            yield();
//            while (true) {
//                gasnet_hsl_lock(lock);
//                if (steal_full[myIndex]) {
//                    res = steal_boxes[myIndex];
//                    gasnet_hsl_unlock(lock);
//                    
//                    if (TREENODE_NULL==res)  // null indicates failure to steal
//                        return false;
//                    
//                    *ele = res;
//                    return true;
//                } else {
//                    // wait for reply
//                    gasnet_hsl_unlock(lock);
//                    yield();
//                }
//            }
//
//        }
//    }
//}
