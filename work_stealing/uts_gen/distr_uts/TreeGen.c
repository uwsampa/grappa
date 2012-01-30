#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "uts.h"
#include "StealQueue.h"
#include "thread.h"
#include "global_array.h"
#include "balloc.h"
#include <omp.h>
#include <math.h>

#include <list>
#include <iterator>

#include "getput.h"
#include "gasnet_cbarrier.h"
#include "collective.h"

#define WAIT_BARRIER gasnet_barrier_notify(11, 0); gasnet_barrier_wait(11, 0)

#define PARALLEL 1
#define PREFETCH_WORK 1
#define PREFETCH_CHILDREN 1
#define DISTRIBUTE_INITIAL 1
#define GEN_TREE_NBI 1
#define CHECK_SERIALIZE 0

#define YIELD_ALARM 0
#if YIELD_ALARM
    //#define TICKS 2600
    //#define TICKS 5200
    #define TICKS 7800
    #define YIELD(me) thread_yield_alarm((me), TICKS)
#else
    #define YIELD(me) thread_yield((me))
#endif

#define member_size(type, member) sizeof(((type *)0)->member)


gasnet_handlerentry_t   handlers[] =
    {
        { GM_REQUEST_HANDLER, (void (*) ()) gm_request_handler },
        { GM_RESPONSE_HANDLER, (void (*) ()) gm_response_handler },
        { GA_HANDLER, (void (*) ()) ga_handler },
        { WORKSTEAL_REQUEST_HANDLER, (void (*) ()) workStealRequestHandler },
        { WORKSTEAL_REPLY_HANDLER, (void (*) ()) workStealReplyHandler },
        { PUSHWORK_REQUEST_HANDLER, (void (*) ()) pushWorkRequestHandler },
        { ENTER_CBARRIER_REQUEST_HANDLER, (void (*) ()) enter_cbarrier_request_handler },
        { EXIT_CBARRIER_REQUEST_HANDLER, (void (*) ()) exit_cbarrier_request_handler },
        { CANCEL_CBARRIER_REQUEST_HANDLER, (void (*) ()) cancel_cbarrier_request_handler },
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

/***********************************************************
 *  Parallel execution parameters                          *
 ***********************************************************/

int doSteal   = 1;//PARALLEL; // 1 => use work stealing
int chunkSize = 10;       // number of nodes to move to/from shared area
int cbint     = 1;        // Cancellable barrier polling interval

int num_cores = 1;
int num_threads_per_core = 2;
int num_places = 1;
int num_procs = 1;
int generateFile = 1;
char* genFilename = "default";
const char* gf_tree_suffix = "tree";
const char* gf_child_suffix = "child";

/* Implementation Specific Functions */
char * impl_getName() { return "TreeGen"; }
int    impl_paramsToStr(char *strBuf, int ind) {
    ind += sprintf(strBuf+ind, "Execution strategy:  ");
    
    //(parallel only)
    ind += sprintf(strBuf+ind, "Parallel search using %d cores\n", num_cores);
    ind += sprintf(strBuf+ind, "    %d threads per core\n", num_threads_per_core);
    ind += sprintf(strBuf+ind, "%d places\n", num_places);
    if (doSteal) {
        ind += sprintf(strBuf+ind, "    Load balance by work stealing, chunk size = %d nodes\n", chunkSize);
        ind += sprintf(strBuf+ind, "  CBarrier Interval: %d\n", cbint);
    } else {
      #if DISTRIBUTE_INITIAL
      ind += sprintf(strBuf+ind, "   Initial distribution only.\n");
      #else
      ind += sprintf(strBuf+ind, "   No load balancing.\n");
      #endif
    }

    return ind;
}

void   impl_abort(int err) { exit(err); }
void   impl_helpMessage() { 
    printf("    -n  int   number of cores\n");
    printf("    -T  int   number of threads per core\n");
    printf("    -c  int   chunksize for work stealing\n");
    printf("    -i  int   set cancellable barrier polling interval\n");
    printf("    -P  int   number of places\n");
    printf("    -p  int   number of process\n");
    printf("    -G  int   0:read, 1:gen-and-write\n");
    printf("    -F  str   generated tree file name\n");

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
        case 'P':
            num_places = atoi(value);
            break;
        case 'p':
            num_procs = atoi(value);
            break;
        case 'F':
            genFilename = value;
            break;
        case 'G':
            generateFile = atoi(value);
            break;
        default:
            err = 1;
            break;
    }

    return err;
}
/* ************************************** */


void __sched__noop__(pid_t pid, unsigned int x, cpu_set_t* y) {}
#define PIN_THREADS 1
#if PIN_THREADS
    #define SCHED_SET sched_setaffinity
#else
    #define SCHED_SET __sched__noop__
#endif
  
#define IBT_PERMUTE 0

int generateTree(Node_ptr root, global_array* nodes, int cid, global_array* child_array, ballocator_t* bals[], struct state_t* states, std::list<Node_ptr> initialNodes);

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
    int num_local_nodes;
    int* work_done;
    global_array* nodes_array;
    global_array* children_arrays;
    int my_id;
    int rank;
    int* neighbors;
} worker_info;



/* search other threads for work to steal */
/* CURRENTLY UNUSED */
//int findwork(int k, int my_id, int num_queues) {
//  int i,v;
//  for (i = 1; i < num_queues; i++) { // TODO permutation order
//    v = (my_id + i) % num_queues;
////#ifdef _SHMEM
////    GET(stealStack[v]->workAvail, stealStack[v]->workAvail, v);
////#endif
//    if (stealStacks[v].workAvail >= k)
//      return v;
//  }
//  return -1;
//}



void releaseNodes(StealStack *ss){
  if (doSteal) {
    if (ss_localDepth(ss) > 2 * chunkSize) {
      // Attribute this time to runtime overhead
/*      ss_setState(ss, SS_OVH);                    */
      ss_release(ss, chunkSize);
      // This has significant overhead on clusters!
      if (ss->nNodes % cbint == 0) { // possible for cbint to get skipped if push multiple?
/*        ss_setState(ss, SS_CBOVH);                */
        cbarrier_cancel();
      }

/*      ss_setState(ss, SS_WORK);                   */
    }
  }
}


void workLoop(StealStack* ss, thread* me, int* work_done, int core_id, int num_local_nodes, global_array* nodes_array, global_array* children_arrays, int my_id, int rank, int* neighbors) {
    while (!(*work_done)) {
            //printf("core %d check work when local=%d\n", core_id, ss->top-ss->local);
        while (ss_localDepth(ss) > 0) {
/*            ss_setState(ss, SS_WORK);        */
            /* get node at stack top */
            Node_ptr work = ss_top(ss);
            //printf("(r%d,th%d) work popped %lu\n", work);
            ss_pop(ss);
            //printf("core %d work(#%lu, id=%d, nc=%d, height=%d)\n", core_id, *nVisited, work->id, work->numChildren, work->height);

            Node workLocal;
          //TODO the get nb causes action that will wake up yielded thread  
            #if PREFETCH_WORK
            // pull in numChildren and children
            mem_tag_t tag = get_remote_nb(nodes_array, work, sizeof(int)+sizeof(Node_ptr_ptr), &workLocal);
            //printf("(r%d,th%d) yield work\n", my_id, me->id);
            YIELD(me); // TODO try cacheline align the nodes (2 per line probably)
            complete_nb(me, tag);
            #endif

            //printf("(r%d,th%d) work popped (has %d children)\n", my_id, me->id, workLocal.numChildren);

            Node_ptr childrenLocal[workLocal.numChildren];
            
            #if PREFETCH_CHILDREN
            tag = get_remote_nb(children_arrays, workLocal.children, workLocal.numChildren*sizeof(Node_ptr), &childrenLocal);
            //printf("(r%d,th%d) yield children\n", my_id, me->id);
            YIELD(me);
            complete_nb(me, tag);
            #endif
            ///TODO

            get_remote(nodes_array, work, offsetof(Node, id), sizeof(workLocal.id), &workLocal.id);
           // printf("r%d got %d(ptr=%lu) (count=%d) pushed\n", rank, workLocal.id, work, ss->nVisited);

            for (int i=0; i<workLocal.numChildren; i++) {
                //printf("core %d pushes child(id=%d,height=%d,parent=%d)\n", core_id, work->children[i]->id, work->children[i]->height, work->id);
                //printf("(r%d,th%d) child pushed %lu\n", my_id, me->id, childrenLocal[i]);
                ss_push(ss, childrenLocal[i]);
             //   printf("%d-->(ptr=%lu)\n", rank, childrenLocal[i]);
                //printf("rank%d: %d nodes visited; just pushed %lu\n", rank, myStealStack.nNodes, childrenLocal[i]);
            }

            // notify other coroutines in my scheduler
            // TODO: may choose to optimize this based on amount in queue
            threads_wakeAll(me);
            //threads_wakeN(me, work->numChildren-1); //based on releasing maybe less
            
            // possibly make work visible and notfiy idle workers 
            releaseNodes(ss);

        }
       
        // try to put some work back to local 
        if (ss_acquire(ss, chunkSize))
            //threads_wakeN(me, chunkSize-1);
            continue;

        // try to steal
        // TODO include alternate version that controls steals with scheduler
        if (doSteal) {
          int goodSteal = 0;
          int victimId;
          
/*          ss_setState(ss, SS_SEARCH);             */
          for (int i = 1; i < num_local_nodes && !goodSteal; i++) { // TODO permutation order
            victimId = (my_id + i) % num_local_nodes;
            goodSteal = ss_steal_locally(ss, neighbors[victimId], chunkSize);
          }

          if (goodSteal) {
              //printf("%d steals %d items\n", omp_get_thread_num(), chunkSize);
              threads_wakeAll(me); // TODO optimize wake based on amount stolen
              continue;
          }

          /**TODO remote load balance**/
        }

        // no work so suggest barrier
        //printf("(r%d,th%d) yield_wait (%p)\n", my_id, me->id, me);
        if (!thread_yield_wait(me)) {
            if (cbarrier_wait()) {
                *work_done = 1;
            }
        }
    }
        
}

void thread_runnable(thread* me, void* arg) {
    struct worker_info* info = (struct worker_info*) arg;

    //printf("thread starts from core %d/%d\n", info->core_id, info->num_cores);
    workLoop(&myStealStack, me, info->work_done, info->core_id, info->num_local_nodes, info->nodes_array, info->children_arrays, info->my_id, info->rank, info->neighbors);

    thread_exit(me, NULL);
}

//XXX
// tree T1
int numNodes = 4130071;
//int numNodes = 102181082;
const int children_amounts[] = {688629, 684065, 687025, 689225, 687816, 693310}; // 6 procs
const int children_array_size = 693310;
// tree T1L
//int numNodes = 102181082;


static const int LARGEPRIME = 961748941;
uint64_t Permute(int i) {
  #if IBT_PERMUTE
    long long int j = i;
    return (j*LARGEPRIME) % numNodes; 
  #else
    return i;
  #endif
}

int generateTree(Node_ptr root, global_array* nodes, int cid, global_array* child_array, ballocator_t* bals[], struct state_t* states, std::list<Node_ptr>* initialNodes) {
//printf("generate cid=%d\n", cid);
    Node rootLocal;
    get_remote(nodes, root, 0, sizeof(Node), &rootLocal);

    // pure functions
    int nc = uts_numChildren(&rootLocal, &states[cid]);
    int childType = uts_childType(&rootLocal);
   
    Node rootTemp;
    rootTemp.type = rootLocal.type;  //copy (not needed if dont overwrite)
    rootTemp.height = rootLocal.height; // copy (not needed if dont overwrite)
    rootTemp.children = (Node_ptr_ptr) balloc(bals[ga_node(nodes, root)], nc);
    rootTemp.numChildren = nc;
    if (cid ==0) {printf("root has %d children\n", nc);}
    rootTemp.id = cid;
  
    #if GEN_TREE_NBI
    put_remote_nbi(nodes, root, &rootTemp.numChildren, offsetof(Node, numChildren), sizeof(rootTemp.numChildren));
    put_remote_nbi(nodes, root, &rootTemp.id, offsetof(Node, id), sizeof(rootTemp.id));
    put_remote_nbi(nodes, root, &rootTemp.children, offsetof(Node, children), sizeof(rootTemp.children));
    #else
    put_remote(nodes, root, &rootTemp, 0, sizeof(Node));
    #endif
//    for (int i=0; i<root->height; i++) {
//        printf("-");
//    }
    //printf("node %d with numc=%d\n", cid, nc);
  //  printf("have %d children\n", nc);
    //printf("%d\n",nc);
   
    int current_cid = cid+1;
   // Node_ptr childrenAddrs[nc];   // assumption that an array lie on a single machine
    for (int i=0; i<nc; i++) {
        uint64_t index = Permute(current_cid);
//        printf("--index=%d\n", index);
//        childrenAddrs[i] = index;
    #if GEN_TREE_NBI
        put_remote_nbi(child_array, rootTemp.children, &index, i*sizeof(Node_ptr), sizeof(Node_ptr));
    #else 
        // TODO could write all 'nc' entries in children array at once if change the cid assignment to not be depth first (which builds up childrenAddrs arrays)
        put_remote(child_array, rootTemp.children, &index, i*sizeof(Node_ptr), sizeof(Node_ptr));
    #endif
       
        Node childTemp;
        childTemp.height = rootTemp.height+1;  
        childTemp.type = childType;
        #if GEN_TREE_NBI
        put_remote_nbi(nodes, index, &childTemp.height, offsetof(Node, height), sizeof(childTemp.height));
        put_remote_nbi(nodes, index, &childTemp.type, offsetof(Node, type), sizeof(childTemp.type));
        #else
        put_remote(nodes, index, &childTemp, 0, sizeof(Node)); // comment out this initialization write, to be faster
        #endif
        
        for (int j=0; j<computeGranularity; j++) {
            rng_spawn(states[rootTemp.id].state, states[current_cid].state, i);
        }

        #if DISTRIBUTE_INITIAL
        if (cid == 0) {
            initialNodes->push_back(index);
        }
        #endif
       
        current_cid = generateTree(index, nodes, current_cid, child_array, bals, states, NULL);    
    }
   
    return current_cid;
}

#include <unistd.h>

typedef uint64_t Node_ptr;
int main(int argc, char *argv[]) {
    init(&argc, &argv);
    
    int rank = gasnet_mynode();
    int num_nodes = gasnet_nodes();
    
    //use uts to parse params
    uts_parseParams(argc, argv, rank==0);
    uts_printParams();

    char hostname[1024];
    gethostname(hostname, 1024);
    printf("rank%d is on %s\n", rank, hostname);

    /** Only for 1 or 2 machines in round-robin placement **/
    int num_machines = 1; //num_places
    int machine_id = rank%num_machines;
    int neighbors[12];
    if ( num_machines == 1) {
        neighbors = {0,1,2,3,4,5,6,7,8,9,10,11};
    } else if ( num_machines == 2) {
         if (machine_id) neighbors= {1,3,5,7,9,11}; else neighbors = {0,2,4,6,8,10};
    }
    int num_local_nodes = num_nodes/num_machines;
    int local_id = rank/num_machines;
    /** **/


    cbarrier_init(num_nodes, rank);
    
    WAIT_BARRIER;

    
    global_array* nodes = ga_allocate(sizeof(Node), numNodes*num_procs); //XXX for enough space
    global_array* children_array_pool = ga_allocate(sizeof(Node_ptr), numNodes*num_procs); //children_array_size*num_procs);
   
   
    if (0 == rank) { printf("v_per_node = %lu\ncha_per_node = %lu\n", nodes->elements_per_node, children_array_pool->elements_per_node);}
    
    Node_ptr root = 0;
    ballocator_t* bals[num_procs]; // for generating tree single threaded
 
    uint64_t num_genNodes;
    std::list<Node_ptr>* initialNodes;
    #if DISTRIBUTE_INITIAL
    initialNodes = new std::list<Node_ptr>();
    #endif
  
    if (generateFile || CHECK_SERIALIZE) {
        if (0 == rank) {
            for (int p=0; p<num_procs; p++) {
                bals[p] = newBumpAllocator(children_array_pool, p*(children_array_pool->elements_per_node), children_array_pool->elements_per_node); 
            }
       
            struct state_t* states = (struct state_t*)malloc(sizeof(struct state_t)*numNodes);
        
            Node rootTemplate;  
            uts_initRoot(&rootTemplate, type, &states[0]);
            put_remote(nodes, root, &rootTemplate, 0, sizeof(Node));
          
            num_genNodes = generateTree(root, nodes, 0, children_array_pool, bals, states, initialNodes);
        
            #if GEN_TREE_NBI
            gasnet_wait_syncnbi_puts(); 
            #endif
            
            free(states);
           
        }
    }

    WAIT_BARRIER;

    if (generateFile) { 
        // write the nodes file
        char fname[256];
        sprintf(fname, "%s.%s.%d", genFilename, gf_tree_suffix, rank);
        FILE* rankfile = fopen(fname, "w");
        global_address gaddr;
        ga_index(nodes, nodes->elements_per_node*rank, &gaddr);
        assert (gm_is_address_local(&gaddr));
        void* v = gm_local_gm_address_to_local_ptr(&gaddr);
        fwrite(v, sizeof(Node), nodes->elements_per_node, rankfile);
        fclose(rankfile);

        // write the child array file
        sprintf(fname, "%s.%s.%d", genFilename, gf_child_suffix, rank);
        rankfile = fopen(fname, "w");
        ga_index(children_array_pool, children_array_pool->elements_per_node*rank, &gaddr);
        assert (gm_is_address_local(&gaddr));
        v = gm_local_gm_address_to_local_ptr(&gaddr);
        fwrite(v, sizeof(Node_ptr), children_array_pool->elements_per_node, rankfile);
        fclose(rankfile);

        // write meta data file
        if ( 0 == rank ) {
            sprintf(fname, "%s.meta", genFilename);
            FILE* metafile = fopen(fname, "w");
            fwrite(&num_genNodes, sizeof(uint64_t), 1, metafile);

            #if DISTRIBUTE_INITIAL
            for (std::list<Node_ptr>::iterator it = initialNodes->begin(); it!=initialNodes->end(); ++it) {
                Node_ptr data = *it;
                fwrite(&data, sizeof(Node_ptr), 1, metafile);
            }
            Node_ptr end = 0; // assumes root is 0 and not pushed
            fwrite(&end, sizeof(Node_ptr), 1, metafile);
            #endif

            fclose(metafile);
        }
    } else { // read file
        //read nodes file
        char fname[256];
        sprintf(fname, "%s.%s.%d", genFilename, gf_tree_suffix, rank);
        FILE* rankfile = fopen(fname, "r");
        global_address gaddr;
        ga_index(nodes, nodes->elements_per_node*rank, &gaddr);
        assert (gm_is_address_local(&gaddr));
        void* v = gm_local_gm_address_to_local_ptr(&gaddr);
        
        #if CHECK_SERIALIZE
        void* vv = malloc(sizeof(Node)*nodes->elements_per_node);
        fread(vv, sizeof(Node), nodes->elements_per_node, rankfile);
        #else
        fread(v, sizeof(Node), nodes->elements_per_node, rankfile);
        #endif
        
        fclose(rankfile);
       
        #if CHECK_SERIALIZE
        printf("%d checking\n", rank);
        Node* genvn = (Node*)v;
        Node* readvn = (Node*)vv;
        for (int i=0; i<nodes->elements_per_node; i++) {
            if (readvn[i].children != genvn[i].children
            ||readvn[i].numChildren != genvn[i].numChildren
            ||readvn[i].type != genvn[i].type
            ||readvn[i].height != genvn[i].height
            ||readvn[i].id != genvn[i].id) {
                printf("rank%d: index %d inconsistent\n", rank, i);
            }
        }
        #endif
        
        // read the child array file
        sprintf(fname, "%s.%s.%d", genFilename, gf_child_suffix, rank);
        rankfile = fopen(fname, "r");
        ga_index(children_array_pool, children_array_pool->elements_per_node*rank, &gaddr);
        assert (gm_is_address_local(&gaddr));
        v = gm_local_gm_address_to_local_ptr(&gaddr);
        
        #if CHECK_SERIALIZE
        vv = malloc(sizeof(Node_ptr)*children_array_pool->elements_per_node);
        fread(vv, sizeof(Node_ptr), children_array_pool->elements_per_node, rankfile);
        #else
        fread(v, sizeof(Node_ptr), children_array_pool->elements_per_node, rankfile);
        #endif
        
        fclose(rankfile);

        #if CHECK_SERIALIZE
        Node_ptr* gencn = (Node_ptr*)v;
        Node_ptr* readcn = (Node_ptr*)vv;
        for (int i=0; i<children_array_pool->elements_per_node; i++) {
            if (readcn[i] != gencn[i]) {
                printf("rank%d: index %d inconsistent\n", rank, i);
            }
        }
        #endif


        // read meta data
        if ( 0 == rank ) {
            sprintf(fname, "%s.meta", genFilename);
            FILE* metafile = fopen(fname, "r");
            fread(&num_genNodes, sizeof(uint64_t), 1, metafile);
            
            #if DISTRIBUTE_INITIAL
            Node_ptr data;
            fread(&data, sizeof(Node_ptr), 1, metafile);
            while (data != 0) {
                initialNodes->push_back(data);
                fread(&data, sizeof(Node_ptr), 1, metafile);
            }
            #endif
            
            fclose(metafile);
        }
    }

    if (0 == rank) printf("num nodes gen: %lu\n", num_genNodes);

    ss_init(&myStealStack, MAXSTACKDEPTH);
    WAIT_BARRIER;

    #if DISTRIBUTE_INITIAL
    if (rank == 0) {
        printf("initial size %d\n", initialNodes->size());
        int current_node = 0;
        while (!initialNodes->empty()) {
            Node_ptr c = initialNodes->front();
            initialNodes->pop_front();
            if (current_node == 0) {
                ss_push(&myStealStack, c);
            } else {
                ss_pushRemote(current_node, &c, 1);
            }
            current_node = (current_node+1)%num_nodes;
        }
    }
    #else
    // push the root
    if (rank == 0) {
        ss_push(&myStealStack, root);
    }
    #endif

    WAIT_BARRIER;
    printf("rank(%d) starting size %d\n", rank, ss_localDepth(&myStealStack));
   
    num_cores = 1; // TODO make 1 always
     
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
            wis[i][th].num_local_nodes = num_local_nodes;
            wis[i][th].work_done = &core_work_done[i];
            wis[i][th].nodes_array = nodes;
            wis[i][th].children_arrays = children_array_pool;
            wis[i][th].my_id = local_id;
            wis[i][th].rank = rank;
            wis[i][th].neighbors = neighbors;
            thread_spawn(master, sched, thread_runnable, &wis[i][th]);
        }
    }


    int coreslist[] = {0,2,4,6,8,10,1,3,5,7,9,11};
  
    WAIT_BARRIER;
   
    double startTime, endTime;
    startTime = uts_wctime();
    
    #pragma omp parallel for num_threads(num_cores)
    for (int core=0; core<num_cores; core++) {
/* TODO Set based on the rank
        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(coreslist[core], &set);
        SCHED_SET(0, sizeof(cpu_set_t), &set);
 */       
        printf("core %d/%d starts\n", core, num_cores);
        run_all(schedulers[core]);

    }

    endTime = uts_wctime();
 WAIT_BARRIER; 
    if (verbose > 2) {
        for (int i=0; i<num_cores; i++) {
            printf("** Thread %d\n", i);
            printf("  # nodes explored    = %d\n", myStealStack.nNodes);
            printf("  # chunks released   = %d\n", myStealStack.nRelease);
            printf("  # chunks reacquired = %d\n", myStealStack.nAcquire);
            printf("  # chunks stolen     = %d\n", myStealStack.nSteal);
            printf("  # failed steals     = %d\n", myStealStack.nFail);
            printf("  max stack depth     = %d\n", myStealStack.maxStackDepth);
            printf("  wakeups             = %d, false wakeups = %d (%.2f%%)",
                    myStealStack.wakeups, myStealStack.falseWakeups,
                    (myStealStack.wakeups == 0) ? 0.00 : ((((double)myStealStack.falseWakeups)/myStealStack.wakeups)*100.0));
            printf("\n");
        }
    }
    
    uint64_t t_nNodes = 0;
    uint64_t t_nRelease = 0;
    uint64_t t_nAcquire = 0;
    uint64_t t_nSteal = 0;
    uint64_t t_nFail = 0;
    uint64_t m_maxStackDepth = 0;

  //  for (int i=0; i<num_cores; i++) {
        t_nNodes += myStealStack.nVisited;
        t_nRelease += myStealStack.nRelease;
        t_nAcquire += myStealStack.nAcquire;
        t_nSteal += myStealStack.nSteal;
        t_nFail += myStealStack.nFail;
        m_maxStackDepth = maxint(m_maxStackDepth, myStealStack.maxStackDepth);
  //  }

   printf("rank(%d) total nodes %lu\n", rank, t_nNodes);
   uint64_t total_nodes = ringReduce(COLL_ADD, 0, t_nNodes);

    if (0 == rank) {
        printf("total visited %lu / %lu\n", total_nodes, num_genNodes);
    }

    if (verbose > 1) {
        if (doSteal) {
            printf("Total chunks released = %d, of which %d reacquired and %d stolen\n",
          t_nRelease, t_nAcquire, t_nSteal);
            printf("Failed steal operations = %d, ", t_nFail);
        }
        
        printf("Max stealStack size = %d\n", m_maxStackDepth);
    }
   
//    uint64_t total_cores_vs[num_cores]; 
//    uint64_t total_visited=0L;
//    #pragma omp parallel for num_threads(num_cores)
//    for (uint64_t i = 0; i<num_cores; i++) {
//        uint64_t total_core_visited = 0;
//    
//        for (int th=0; th<num_threads_per_core; th++) {
//            total_core_visited+=wis[i][th].nVisited;
//        }
//        total_cores_vs[i] = total_core_visited;
//        printf("core %d: visited %lu, steal %d\n", i, total_core_visited, stealStacks[i].nSteal);
//    }
//
//    
//    for (uint64_t i = 0; i<num_cores; i++) {
//        total_visited+=total_cores_vs[i];
//    }
//    printf("total visited %lu / %lu\n", total_visited, num_genNodes);

    double runtime = endTime - startTime;
    double rate = total_nodes / runtime;
   

//TODO: the steal statistics are currently for one node (rank 0)

    if (0 == rank) {
    printf("{'runtime':%f, 'rate':%f, 'num_cores':%d, 'num_threads_per_core':%d, 'chunk_size':%d, 'cbint':%d, 'nVisited':%lu, 'nRelease':%lu, 'nAcquire':%lu, 'nSteal':%lu, 'nFail':%lu, 'maxStackDepth':%lu, 'num_processes':%d, 'num_places':%d}\n", runtime, rate, num_cores, num_threads_per_core, chunkSize, cbint, total_nodes, t_nRelease, t_nAcquire, t_nSteal, t_nFail, m_maxStackDepth, num_nodes, num_places);
    }

}


