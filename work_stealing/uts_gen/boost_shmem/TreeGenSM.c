#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "uts.h"
#include "StealQueue.h"

#include <omp.h>
#include <signal.h>

#include "shmalloc.h"
#include "balloc.h"

#define PARALLEL 1
#define prefetch(x) __builtin_prefetch((x), 0, 1)
#define PREFETCH_WORK 1
#define PREFETCH_CHILDREN 0


#define YIELD_ALARM 0
#if YIELD_ALARM
    //#define TICKS 2600
    //#define TICKS 5200
    #define TICKS 7800
    #define YIELD(me) thread_yield_alarm((me), TICKS)
#else
    #define YIELD(me) thread_yield((me))
#endif

/***********************************************************
 *  Parallel execution parameters                          *
 ***********************************************************/

int doSteal   = PARALLEL; // 1 => use work stealing
int chunkSize = 10;       // number of nodes to move to/from shared area
int cbint     = 1;        // Cancellable barrier polling interval

int num_cores = 4;
int num_threads_per_core = 8;


// termination detection
int cb_cancel;
int cb_count;
int cb_done;
LOCK_T * cb_lock;


/* Implementation Specific Functions */
char * impl_getName() { return "TreeGen"; }
int    impl_paramsToStr(char *strBuf, int ind) {
    ind += sprintf(strBuf+ind, "Execution strategy:  ");
    
    //(parallel only)
    ind += sprintf(strBuf+ind, "Parallel search using %d cores\n", num_cores);
    ind += sprintf(strBuf+ind, "    %d threads per core\n", num_threads_per_core);
    if (doSteal) {
        ind += sprintf(strBuf+ind, "    Load balance by work stealing, chunk size = %d nodes\n", chunkSize);
        ind += sprintf(strBuf+ind, "  CBarrier Interval: %d\n", cbint);
    } else {
      ind += sprintf(strBuf+ind, "   No load balancing.\n");
    }

    return ind;
}

void   impl_abort(int err) { exit(err); }
void   impl_helpMessage() { 
    printf("    -n  int   number of cores\n");
    printf("    -T  int   number of threads per core\n");
    printf("    -c  int   chunksize for work stealing\n");
    printf("    -i  int   set cancellable barrier polling interval\n");
}

int impl_parseParam(char *param, char *value) { 
    int err = 0;

    switch (param[1]) {
        case 'n':
            num_cores = atoi(value);
            break;
        case 'T':
            num_threads_per_core = atoi(value);
            break;
        case 'c':
            chunkSize = atoi(value); 
            break;
        case 'i':
            cbint = atoi(value);
            break;
        default:
            err = 1;
            break;
    }

    return err;
}
/* ************************************** */


int generateTree(Node* root, Node* nodes, int cid, struct state_t* states);


//XXX
// tree T1
//int numNodes = 4130071;
// tree T1L
int numNodes = 102181082;


static const int LARGEPRIME = 961748941;
int Permute(int i) {
  #if IBT_PERMUTE
    long long int j = i;
    return (j*LARGEPRIME) % numNodes; 
  #else
    return i;
  #endif
}
int generateTree(Node* root, Node* nodes, int cid, ballocator_t* ba, struct state_t* states) {
    int nc = uts_numChildren(root, &states[cid]);
    int childType = uts_childType(root);
    
    root->children = (Node**)balloc(ba, sizeof(Node*)*nc);
    root->numChildren = nc;
    root->id = cid;
//    for (int i=0; i<root->height; i++) {
//        printf("-");
//    }
    //printf("node %d with numc=%d\n", cid, nc);
  //  printf("have %d children\n", nc);
    //printf("%d\n",nc);
   
    int current_cid = cid+1;
    for (int i=0; i<nc; i++) {
        int index = Permute(current_cid);
//        printf("--index=%d\n", index);
        root->children[i] = &nodes[index];
        root->children[i]->height = root->height+1;
        root->children[i]->numChildren = -1;
        root->children[i]->type = childType;
        for (int j=0; j<computeGranularity; j++) {
            //rng_spawn(root->state.state, root->children[i]->state.state, i);
            rng_spawn(states[root->id].state, states[current_cid].state, i);
            }

        current_cid = generateTree(root->children[i], nodes, current_cid, ba, states);    
    }
    return current_cid;
}


const char* node_alloc_key  = "_tree_sh_mem_";
void cleanup (int param) {
    printf("Cleaning up sh mem\n");
    shfree(node_alloc_key);
    exit(1);
}

int main(int argc, char *argv[]) {
    
    //use uts to parse params
    uts_parseParams(argc, argv);
    uts_printParams();

    struct state_t* states = (struct state_t*)malloc(sizeof(struct state_t)*numNodes);
     
    shfree(node_alloc_key); // delete in case it exists
    const size_t nodesAllocSize = sizeof(Node)*numNodes;
    const size_t childAllocSize = sizeof(Node*)*numNodes;
    const size_t totalAllocSize = nodesAllocSize + childAllocSize;
    void* shm_allocation = shmalloc(totalAllocSize, node_alloc_key);
    printf("allocation: [%p,%p)", shm_allocation, ((char*)shm_allocation)+totalAllocSize);
    ballocator_t* ba = newBumpAllocator(shm_allocation, totalAllocSize);
    Node* nodes = (Node*) balloc(ba, nodesAllocSize);

    Node* root = &nodes[0];
    uts_initRoot(root, type, &states[0]);
    root->height = 0;
    root->numChildren = -1;
   
    uint64_t num_genNodes = generateTree(root, nodes, 0, ba, states);
    free(states);
    printf("num nodes gen: %lu\n", num_genNodes);
    signal (SIGINT, cleanup);

    printf("may now start workers\n");

    while(true) {};
}
