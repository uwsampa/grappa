#include <stdio.h>
#include <stdlib.h>
#include "uts.h"
#include "StealQueue.h"
#include "thread.h"

#include <omp.h>
#include <sched.h>



#define PARALLEL 1
#define prefetch(x) __builtin_prefetch((x), 0, 1)

/***********************************************************
 *  Parallel execution parameters                          *
 ***********************************************************/

int doSteal   = PARALLEL; // 1 => use work stealing
int chunkSize = 8;       // number of nodes to move to/from shared area
int cbint     = 1;        // Cancellable barrier polling interval


// termination detection
int cb_cancel;
int cb_count;
int cb_done;
LOCK_T * cb_lock;


/* Implementation Specific Functions */
char * impl_getName() { return "TreeGen"; }
int    impl_paramsToStr(char *strBuf, int ind) { return ind; }
int    impl_parseParam(char *param, char *value) { return 1; } //always find no arg
void   impl_helpMessage() { return; }
void   impl_abort(int err) { exit(err); }

void __sched__noop__(pid_t pid, unsigned int x, cpu_set_t* y) {}
#define PIN_THREADS 1
#if PIN_THREADS
    #define SCHED_SET sched_setaffinity
#else
    #define SCHED_SET __sched__noop__
#endif
  
#define IBT_PERMUTE 1

int generateTree(Node* root, Node* nodes, int cid);

//enum   uts_trees_e    { BIN = 0, GEO, HYBRID, BALANCED };
//enum   uts_geoshape_e { LINEAR = 0, EXPDEC, CYCLIC, FIXED };
//
//typedef enum uts_trees_e    tree_t;
//typedef enum uts_geoshape_e geoshape_t;
//
//
//
//double rng_toProb(int n) {
//  if (n < 0) {
//    printf("*** toProb: rand n = %d out of range\n",n);
//  }
//  return ((n<0)? 0.0 : ((double) n)/2147483648.0);
//}


typedef struct worker_info {
    int num_cores_per_node;
    int core_id;
    int num_cores;
    int* work_done;
} worker_info;



/* search other threads for work to steal */
int findwork(int k, int my_id, int num_queues) {
  int i,v;
  for (i = 1; i < num_queues; i++) { // TODO permutation order
    v = (my_id + i) % num_queues;
//#ifdef _SHMEM
//    GET(stealStack[v]->workAvail, stealStack[v]->workAvail, v);
//#endif
    if (stealStacks[v].workAvail >= k)
      return v;
  }
  return -1;
}

void cb_init() {
    INIT_SINGLE_LOCK(cb_lock);

    SET_LOCK(cb_lock);
    cb_count = 0;
    cb_cancel = 0;
    cb_done = 0;
    UNSET_LOCK(cb_lock);
}

int cbarrier_wait(int num_threads) {
    int l_count, l_done, l_cancel;
    
    SET_LOCK(cb_lock);
    cb_count++;
    if (cb_count == num_threads) {
        cb_done = 1;
    }

    l_count = cb_count;
    l_done = cb_done;
    UNSET_LOCK(cb_lock);
    

    do {
        SET_LOCK(cb_lock);
        l_count = cb_count;
        l_cancel = cb_cancel;
        l_done = cb_done;
        UNSET_LOCK(cb_lock);
    } while (!l_cancel && !l_done);

    SET_LOCK(cb_lock);
    cb_count--;
    cb_cancel = 0;
    l_done = cb_done;
    UNSET_LOCK(cb_lock);

    return l_done;
}

void cbarrier_cancel() {
    SET_LOCK(cb_lock);
    cb_cancel = 1;
    UNSET_LOCK(cb_lock);
}

void releaseNodes(StealStack *ss){
  if (doSteal) {
    if (ss_localDepth(ss) > 2 * chunkSize) {
      // Attribute this time to runtime overhead
/*      ss_setState(ss, SS_OVH);                    */
      ss_release(ss, chunkSize);
      // This has significant overhead on clusters!
      if (ss->nNodes % cbint == 0) {
/*        ss_setState(ss, SS_CBOVH);                */
        cbarrier_cancel();
      }

/*      ss_setState(ss, SS_WORK);                   */
    }
  }
}


void workLoop(StealStack* ss, thread* me, int* work_done, int core_id, int num_cores) {

    while (!(*work_done)) {
        while (ss_localDepth(ss) > 0) {
/*            ss_setState(ss, SS_WORK);        */

            /* get node at stack top */
            Node* work = ss_top(ss);
            ss_pop(ss);

            /* DISTR TODO
             * This is where the need to load
             * 'work' node to get numChildren and ptrs
             */
            prefetch(work); // hopefully this pulls in a single cacheline containing numChildren and children
            thread_yield(me); // TODO try cacheline align the nodes (2 per line probably)

            for (int i=0; i<work->numChildren; i++) {
                // TODO also need to pull in work->children array but hopefully for single-node the spatial locality is enough
                ss_push(ss, work->children[i]);
            }

            // notify other coroutines in my scheduler
            // TODO: may choose to optimize this based on amount in queue
            threads_wake(me);
            
            // possibly make work visible and notfiy idle workers 
            releaseNodes(ss);
        }
       
        // try to put some work back to local 
        if (ss_acquire(ss, chunkSize))
            continue;

        // try to steal
        // TODO include alternate version that controls steals with scheduler
        if (doSteal) {
          int goodSteal = 0;
          int victimId;
          
/*          ss_setState(ss, SS_SEARCH);             */
          victimId = findwork(chunkSize, core_id, num_cores);
          while (victimId != -1 && !goodSteal) {
              // some work detected, try to steal it
              goodSteal = ss_steal_locally(ss, victimId, chunkSize);
              // keep trying because work disappeared
              if (!goodSteal)
                  victimId = findwork(chunkSize, core_id, num_cores);
          }
          if (goodSteal) {
              threads_wake(me); // TODO optimize wake based on amount stolen
              continue;
          }
        }

        // no work so suggest barrier
        if (!thread_yield_wait(me)) {
            if (cbarrier_wait(num_cores)) {
                *work_done = 1;
            }
        }

    }
        
}

void thread_runnable(thread* me, void* arg) {
    struct worker_info* info = (struct worker_info*) arg;

    workLoop(&stealStacks[info->core_id], me, info->work_done, info->core_id, info->num_cores);

    thread_exit(me, NULL);
}

//XXX
// tree T1
int numNodes = 4130071;


static const int LARGEPRIME = 961748941;
int Permute(int i) {
  #if IBT_PERMUTE
    long long int j = i;
    return (j*LARGEPRIME) % numNodes; 
  #else
    return i;
  #endif
}

int generateTree(Node* root, Node* nodes, int cid) {
    int nc = uts_numChildren(root);
    int childType = uts_childType(root);
    
    root->children = (Node**)malloc(sizeof(Node*)*nc);
    root->numChildren = nc;
  //  printf("have %d children\n", nc);
    //printf("%d\n",nc);
   
    int current_cid = cid; 
    for (int i=0; i<nc; i++) {
        int index = Permute(current_cid++);
//        printf("--index=%d\n", index);
        root->children[i] = &nodes[index];
        root->children[i]->height = root->height+1;
        root->children[i]->numChildren = -1;
        root->children[i]->type = childType;
        for (int j=0; j<computeGranularity; j++) {
            rng_spawn(root->state.state, root->children[i]->state.state, i);
            }

        current_cid = generateTree(root->children[i], nodes, current_cid);    
    }
    return cid;
}


int main(int argc, char *argv[]) {
    
    //use uts to parse params
    uts_parseParams(argc, argv);
    uts_printParams();
    cb_init();

    Node* nodes = (Node*) malloc(sizeof(Node)*numNodes);
    Node root = nodes[0];
    uts_initRoot(&root, type);
    root.height = 0;
    root.numChildren = -1;
    
    generateTree(&root, nodes, 1);

   
   //TODO arg in impl_paramParse
   int num_cores = 2;
   int num_threads_per_core = 2;
    
    for (int i=0; i<num_cores; i++) {
        ss_init(&stealStacks[i], MAXSTACKDEPTH);
    }

    // push the root
    ss_push(&stealStacks[0], &root);
    
     
    // green threads init
    thread* masters[num_cores];
    scheduler* schedulers[num_cores];
    worker_info wis[num_cores][num_threads_per_core];
    int core_work_done[num_cores];

    #pragma omp parallel for num_threads(num_cores)
    for (uint64_t i = 0; i<num_cores; i++) {
        thread* master = thread_init();
        scheduler* sched = create_scheduler(master);
        masters[i] = master;
        schedulers[i] = sched;
        core_work_done[i] = 0;

        for (int th=0; th<num_threads_per_core; th++) {
            wis[i][th].num_cores_per_node = num_cores;
            wis[i][th].core_id = i;
            wis[i][th].num_cores = num_cores;
            wis[i][th].work_done = &core_work_done[i];
            thread_spawn(masters[i], schedulers[i], thread_runnable, &wis[i][th]);
        }
    }


    int coreslist[] = {0,2,4,6,8,10,1,3,5,7,9,11};
    
    #pragma omp parallel for num_threads(num_cores)
    for (int core=0; core<num_cores; core++) {

  /*XXX not in sched.h???
        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(coreslist[core], &set);
        SCHED_SET(0, sizeof(cpu_set_t), &set);
*/

        run_all(schedulers[core]);
    }


}
