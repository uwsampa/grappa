#include <iostream>

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "uts.h"
#include "StealQueue.h"
#include "thread.h"
#include "balloc.h"
#include <omp.h>
#include <math.h>

#include <list>
#include <iterator>

#include "gasnet_cbarrier.h"
#include "collective.h"

#include "SoftXMT.hpp"
#include "IncoherentAcquirer.hpp"
#include "IncoherentReleaser.hpp"

#include "getput.h"
#include "global_memory.h"
#include "global_array.h"

#include "DictOut.hpp"

#define PARALLEL 1
#define PREFETCH_WORK 1
#define PREFETCH_CHILDREN 1
#define DISTRIBUTE_INITIAL 1
#define GEN_TREE_NBI 1
#define CHECK_SERIALIZE 0

#define NO_INTEG 0

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

void __sched__noop__(pid_t pid, unsigned int x, cpu_set_t* y) {}
#define PIN_THREADS 1
#if PIN_THREADS
    #define SCHED_SET sched_setaffinity
#else
    #define SCHED_SET __sched__noop__
#endif

/**********************************
 * Gasnet process code
 *********************************/
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


void function_dispatch(int func_id, void *buffer, uint64_t size) {
    }
    
/*********************************/

/***********************************************************
 *  Parallel execution parameters                          *
 ***********************************************************/

int doSteal   = 1;        // 1 => use work stealing
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


  
#define IBT_PERMUTE 0  // 0->layout with sequential addresses,1->pseudorandom addresses

int generateTree(Node_ptr root, global_array* nodes, int cid, global_array* child_array, ballocator_t* bals[], struct state_t* states, std::list<Node_ptr> initialNodes);

typedef struct worker_info {
    int num_workers;
    int num_local_nodes;
    int* work_done;
    int* okToSteal;
    global_array* nodes_array;
    global_array* children_arrays;
    int my_local_id;
    int* neighbors;
} worker_info;


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

// Part of a TreeNode
typedef struct Node_piece_t {
    int numChildren;
    Node_ptr_ptr children;
} Node_piece_t;

void visitTask(Node_ptr work, thread* me, StealStack* ss, global_array* nodes_array, global_array* children_arrays);
    
void worker_thread(StealStack* ss, thread* me, int* work_done, int* okToSteal, int num_workers, int num_local_nodes, global_array* nodes_array, global_array* children_arrays, int my_local_id, int* neighbors) {
    thread_idle(me, num_workers);
    while (!(*work_done)) {
       while (ss_localDepth(ss) > 0) {
           Node_ptr work = ss_top(ss); //TODO generalize
           ss_pop(ss);
           visitTask(work, me, ss, nodes_array, children_arrays);  // TODO generalize
       }
       
       if (doSteal) {
           // try to put some work back to local 
           if (ss_acquire(ss, chunkSize))
               continue;

           if (*okToSteal) {

               // try to steal
               if (doSteal) {
                   int goodSteal = 0;
                   int victimId;

                   /*          ss_setState(ss, SS_SEARCH);             */
                   for (int i = 1; i < num_local_nodes && !goodSteal; i++) { // TODO permutation order
                       victimId = (my_local_id + i) % num_local_nodes;
                       goodSteal = ss_steal_locally(ss, neighbors[victimId], chunkSize);
                   }

                   if (goodSteal) {
                       continue;
                   } else {
                       thread_idlesReady(me, 0); // prevent idle unassigned threads from being scheduled
                       *okToSteal = 0;      // prevent running unassigned threads from trying to steal again
                   }

                   /**TODO remote load balance**/
               }

           }
       }

       if (!thread_idle(me, num_workers)) {
            // no work so suggest barrier
            if (cbarrier_wait()) {
                *work_done = 1;
            }
            thread_idlesReady(me, 1);   // work is available so allow unassigned threads to be scheduled
            *okToSteal = 1;        // work is available so allow steal attempts
        }
    }
}
/** include a isTaskAssigned thread member to assert on at various points? **/
           


void visitTask(Node_ptr work, thread* me, StealStack* ss, global_array* nodes_array, global_array* children_arrays) {
    #if PREFETCH_WORK
    #if NO_INTEG
    TreeNode workLocal;
    // pull in numChildren and children
    mem_tag_t tag = get_remote_nb(nodes_array, work, sizeof(int)+sizeof(Node_ptr_ptr), &workLocal);
    YIELD(me); // TODO try cacheline align the nodes (2 per line probably)
    complete_nb(me, tag);
    #else

    global_address workAddress;
    ga_index(nodes_array, work, &workAddress);
//    Node_piece_t* workLocal = get_local_copy_of_remote<Node_piece_t>(workAddress, 1); 
    Node_piece_t workLocal;
    GlobalAddress< Node_piece_t > workAddress_gl ((Node_piece_t*)gm_address_to_ptr(&workAddress), gm_node_of_address(&workAddress));
    IncoherentAcquirer<Node_piece_t> workLocalAq(workAddress_gl, 1, &workLocal);
    workLocalAq.start_acquire();
    workLocalAq.block_until_acquired();
    #endif
    #endif


    #if PREFETCH_CHILDREN
    #if NO_INTEG
    Node_ptr childrenLocal[workLocal.numChildren];
    tag = get_remote_nb(children_arrays, workLocal.children, workLocal.numChildren*sizeof(Node_ptr), &childrenLocal);
    YIELD(me);
    complete_nb(me, tag);

    #else

    global_address childAddress;
    ga_index(children_arrays, workLocal.children, &childAddress);
//    Node_ptr* childrenLocal = get_local_copy_of_remote<Node_ptr>(childAddress, workLocal->numChildren);

    Node_ptr childrenLocal[workLocal.numChildren];
    GlobalAddress< Node_ptr > childAddress_gl ((Node_ptr*)gm_address_to_ptr(&childAddress), gm_node_of_address(&childAddress));
    IncoherentAcquirer<Node_ptr> childrenLocalAq(childAddress_gl, workLocal.numChildren, childrenLocal);
    childrenLocalAq.start_acquire();
    childrenLocalAq.block_until_acquired();
    #endif
    #endif

    for (int i=0; i<workLocal.numChildren; i++) {
        ss_push(ss, childrenLocal[i]);
    }



   
   
    #if NO_INTEG
    // notify other coroutines in my scheduler
    
    
    #else
    IncoherentReleaser<Node_ptr> childrenLocalRl( childAddress_gl, workLocal.numChildren, childrenLocal );
    childrenLocalRl.start_release();

    IncoherentReleaser<Node_piece_t> workLocalRl( workAddress_gl, 1, &workLocal );
    workLocalRl.start_release();


    #endif
    
    //TODO: notifcations below should be encapsulated, not part of task code
    
    // allow idle threads to be scheduled now that there
    // may be work
    thread_idlesReady(me, 1);
    
    // possibly make work visible and notfiy idle workers 
    releaseNodes(ss);

    #if NO_INTEG
    #else
    childrenLocalRl.block_until_released();
    workLocalRl.block_until_released();
    #endif
}

void thread_runnable(thread* me, void* arg) {
    struct worker_info* info = (struct worker_info*) arg;

    // TODO: nodes_array, children_arrays args should not be told directly to worker
    worker_thread(&myStealStack, me, info->work_done, info->okToSteal, info->num_workers, info->num_local_nodes, info->nodes_array, info->children_arrays, info->my_local_id, info->neighbors);

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
    TreeNode rootLocal;
    get_remote(nodes, root, 0, sizeof(TreeNode), &rootLocal);

    // pure functions
    int nc = uts_numChildren(&rootLocal, &states[cid]);
    int childType = uts_childType(&rootLocal);
   
    TreeNode rootTemp;
    rootTemp.type = rootLocal.type;  //copy (not needed if dont overwrite)
    rootTemp.height = rootLocal.height; // copy (not needed if dont overwrite)
    rootTemp.children = (Node_ptr_ptr) balloc(bals[ga_node(nodes, root)], nc);
    rootTemp.numChildren = nc;
    if (cid ==0) {printf("root has %d children\n", nc);}
    rootTemp.id = cid;
  
    #if GEN_TREE_NBI
    put_remote_nbi(nodes, root, &rootTemp.numChildren, offsetof(TreeNode, numChildren), sizeof(rootTemp.numChildren));
    put_remote_nbi(nodes, root, &rootTemp.id, offsetof(TreeNode, id), sizeof(rootTemp.id));
    put_remote_nbi(nodes, root, &rootTemp.children, offsetof(TreeNode, children), sizeof(rootTemp.children));
    #else
    put_remote(nodes, root, &rootTemp, 0, sizeof(TreeNode));
    #endif
   
    int current_cid = cid+1;
   // Node_ptr childrenAddrs[nc];   // assumption that an array lie on a single machine
    for (int i=0; i<nc; i++) {
        uint64_t index = Permute(current_cid);
    #if GEN_TREE_NBI
        put_remote_nbi(child_array, rootTemp.children, &index, i*sizeof(Node_ptr), sizeof(Node_ptr));
    #else 
        // TODO could write all 'nc' entries in children array at once if change the cid assignment to not be depth first (which builds up childrenAddrs arrays)
        put_remote(child_array, rootTemp.children, &index, i*sizeof(Node_ptr), sizeof(Node_ptr));
    #endif
       
        TreeNode childTemp;
        childTemp.height = rootTemp.height+1;  
        childTemp.type = childType;
        #if GEN_TREE_NBI
        put_remote_nbi(nodes, index, &childTemp.height, offsetof(TreeNode, height), sizeof(childTemp.height));
        put_remote_nbi(nodes, index, &childTemp.type, offsetof(TreeNode, type), sizeof(childTemp.type));
        #else
        put_remote(nodes, index, &childTemp, 0, sizeof(TreeNode)); // comment out this initialization write, to be faster
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

struct user_main_args {
    int argc;
    char** argv;
    global_array* nodes;
    global_array* children_array_pool;
};

void user_main( thread* me, void* args );

typedef uint64_t Node_ptr;
int main(int argc, char *argv[]) {
    SoftXMT_init(&argc, &argv);

    SoftXMT_activate();

    
    ////////////////////////////////////////////////
    // Doing shared allocation in SPMD land right now
    ////////////////////////////////////////////////
    
    // FIXME: remove once global memory is impl in SoftXMT
    gm_init();
    
    int rank = SoftXMT_mynode();
    int num_nodes = SoftXMT_nodes();
    
    global_array* nodes = ga_allocate(sizeof(TreeNode), numNodes*num_procs); //XXX for enough space
    global_array* children_array_pool = ga_allocate(sizeof(Node_ptr), numNodes*num_procs); //children_array_size*num_procs);
    
    
    SoftXMT_barrier();
    ////////////////////////////////////////////////////////
    
    user_main_args um_args = { argc, argv, nodes, children_array_pool };  
    SoftXMT_run_user_main( &user_main, &um_args );

    SoftXMT_finish( 0 );
}
   
   
struct init_thread_args {
    uint64_t num_genNodes;
    global_array* nodes;
    global_array* children_array_pool;
    std::list<Node_ptr>* initialNodes;
    double startTime;
    double endTime;
};
   
void init_thread_f(thread* me, void* args ) {
    init_thread_args* iargs = (init_thread_args*) args;
    
    int num_nodes = SoftXMT_nodes();
    int rank = SoftXMT_mynode(); 
    
    uint64_t num_genNodes = iargs->num_genNodes;
    global_array* nodes = iargs->nodes;
    global_array* children_array_pool = iargs->children_array_pool;
    std::list<Node_ptr>* initialNodes = iargs->initialNodes;
    
    int num_local_nodes = num_nodes/num_places;
    assert( num_local_nodes * num_places == num_nodes ); // For now: only allow equal processes per machine
    int local_id = rank/num_places;

    int machine_id = rank % num_places;
    int neighbors[12];
    if ( num_places == 1) {
        neighbors = {0,1,2,3,4,5,6,7,8,9,10,11};
    } else if ( num_places == 2) {
         if (machine_id) neighbors= {1,3,5,7,9,11}; else neighbors = {0,2,4,6,8,10};
    }

    cbarrier_init(num_nodes, rank);
    
    if (generateFile) { 
        // write the nodes file
        char fname[256];
        sprintf(fname, "%s.%s.%d", genFilename, gf_tree_suffix, rank);
        FILE* rankfile = fopen(fname, "w");
        global_address gaddr;
        ga_index(nodes, nodes->elements_per_node*rank, &gaddr);
        assert (gm_is_address_local(&gaddr));
        void* v = gm_local_gm_address_to_local_ptr(&gaddr);
        fwrite(v, sizeof(TreeNode), nodes->elements_per_node, rankfile);
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
        void* vv = malloc(sizeof(TreeNode)*nodes->elements_per_node);
        fread(vv, sizeof(TreeNode), nodes->elements_per_node, rankfile);
        #else
        fread(v, sizeof(TreeNode), nodes->elements_per_node, rankfile);
        #endif
        
        fclose(rankfile);
       
        #if CHECK_SERIALIZE
        printf("%d checking\n", rank);
        TreeNode* genvn = (TreeNode*)v;
        TreeNode* readvn = (TreeNode*)vv;
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
    printf("rank(%d) starting size %d\n", rank, ss_localDepth(&myStealStack));
   
  
    
    /* spawn workers */
    worker_info wis[num_threads_per_core];
    int core_work_done;
    int core_okToSteal;
    thread* worker_threads[num_threads_per_core];

    for (int th=0; th<num_threads_per_core; th++) {
        wis[th].num_local_nodes = num_local_nodes;
        wis[th].num_workers = num_threads_per_core;
        wis[th].work_done = &core_work_done;
        wis[th].okToSteal = &core_okToSteal;
        wis[th].nodes_array = nodes;
        wis[th].children_arrays = children_array_pool;
        wis[th].my_local_id = local_id;
        wis[th].neighbors = neighbors;
        worker_threads[th] = SoftXMT_spawn(thread_runnable, &wis[th]);
    }
    
    /*
    int coreslist[] = {0,2,4,6,8,10,1,3,5,7,9,11};
    //TODO Set based on the rank
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(coreslist[core], &set);
    SCHED_SET(0, sizeof(cpu_set_t), &set);
    */       
    


    SoftXMT_barrier(); // no communication yet so this is safe
    
    iargs->startTime = uts_wctime();
    
    /* join on workers (or any 1 worker since sync will be in work loop) */
    for (int th=0; th<num_threads_per_core; th++) {
        SoftXMT_join(worker_threads[th]);
    }

    iargs->endTime = uts_wctime();

    // count nodes
    uint64_t t_nNodes = myStealStack.nVisited;

    printf("rank(%d) total nodes %lu\n", rank, t_nNodes);
    uint64_t total_nodes = ringReduce(COLL_ADD, 0, t_nNodes);
}

//space for init_thread args for this node
init_thread_args this_node_iargs; 

// AM for spawning remote thread
void spawn_initthread_am( init_thread_args* args, size_t size, void* payload, size_t payload_size ) {
   /* in general (for async am handling) this may need synchronization */
   memcpy(&this_node_iargs, args, size);
   SoftXMT_spawn(&init_thread_f, &this_node_iargs); 
}

void user_main( thread* me, void* args) {
    user_main_args* umargs = (user_main_args*) args;
    int argc = umargs->argc;
    char** argv = umargs->argv;
    global_array* nodes = umargs->nodes;
    global_array* children_array_pool = umargs->children_array_pool;
     
    Node rank = SoftXMT_mynode();
    int num_nodes = SoftXMT_nodes();
    
    //use uts to parse params
    uts_parseParams(argc, argv, rank==0);
    uts_printParams();

    char hostname[1024];
    gethostname(hostname, 1024);
    printf("rank%d is on %s\n", rank, hostname);


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
        
            TreeNode rootTemplate;  
            uts_initRoot(&rootTemplate, type, &states[0]);
            put_remote(nodes, root, &rootTemplate, 0, sizeof(TreeNode));
          
            num_genNodes = generateTree(root, nodes, 0, children_array_pool, bals, states, initialNodes);
        
            #if GEN_TREE_NBI
            gasnet_wait_syncnbi_puts(); 
            #endif
            
            free(states);
        }
    }


    thread* init_thread;
    init_thread_args my_iargs;
    for (int nod = 0; nod<num_nodes; nod++) {
        if (nod == 0) {
            init_thread_args my_iargs; 
            my_iargs.num_genNodes = num_genNodes;
            my_iargs.nodes = nodes;
            my_iargs.children_array_pool = children_array_pool;
            my_iargs.initialNodes = initialNodes;
            my_iargs.startTime = 0;
            my_iargs.endTime = 0;
            init_thread = SoftXMT_spawn(&init_thread_f, &my_iargs);
        } else {
            init_thread_args iargs = { 
              num_genNodes,
              nodes,
              children_array_pool,
              initialNodes,
              0,
              0,
            };
            SoftXMT_call_on( nod, &spawn_initthread_am, &iargs );
        }
    }
    SoftXMT_join(init_thread);



//TODO: finish Vlogging
//    if (verbose > 2) {
//        for (int i=0; i<num_cores; i++) {
//            VLOG("** Thread %d\n", i);
//            VLOG("  # nodes explored    = %d\n", myStealStack.nNodes);
//            VLOG("  # chunks released   = %d\n", myStealStack.nRelease);
//            VLOG("  # chunks reacquired = %d\n", myStealStack.nAcquire);
//            VLOG("  # chunks stolen     = %d\n", myStealStack.nSteal);
//            VLOG("  # failed steals     = %d\n", myStealStack.nFail);
//            VLOG("  max stack depth     = %d\n", myStealStack.maxStackDepth);
//            VLOG("  wakeups             = %d, false wakeups = %d (%.2f%%)",
//                    myStealStack.wakeups, myStealStack.falseWakeups,
//                    (myStealStack.wakeups == 0) ? 0.00 : ((((double)myStealStack.falseWakeups)/myStealStack.wakeups)*100.0));
//            printf("\n");
//        }
//    }
    
    uint64_t t_nNodes = 0;
    uint64_t t_nRelease = 0;
    uint64_t t_nAcquire = 0;
    uint64_t t_nSteal = 0;
    uint64_t t_nFail = 0;
    uint64_t m_maxStackDepth = 0;

    t_nRelease += myStealStack.nRelease;
    t_nAcquire += myStealStack.nAcquire;
    t_nSteal += myStealStack.nSteal;
    t_nFail += myStealStack.nFail;
    m_maxStackDepth = maxint(m_maxStackDepth, myStealStack.maxStackDepth);


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

    double runtime = my_iargs.endTime - my_iargs.startTime;
    double rate = total_nodes / runtime;
   

//TODO: the steal statistics are currently for one node (rank 0)

    if (0 == rank) {
       DictOut outputs;
       outputs.add( "runtime", runtime );
       outputs.add( "rate", rate );
       outputs.add( "num_threads", num_threads_per_core );
       outputs.add( "chunk_size", chunkSize );
       outputs.add( "cbint", cbint );
       
       std::cout << outputs.toString() << std::endl;   
    
    //printf("{'runtime':%f, 'rate':%f, 'num_cores':%d, 'num_threads_per_core':%d, 'chunk_size':%d, 'cbint':%d, 'nVisited':%lu, 'nRelease':%lu, 'nAcquire':%lu, 'nSteal':%lu, 'nFail':%lu, 'maxStackDepth':%lu, 'num_processes':%d, 'num_places':%d}\n", runtime, rate, num_cores, num_threads_per_core, chunkSize, cbint, total_nodes, t_nRelease, t_nAcquire, t_nSteal, t_nFail, m_maxStackDepth, num_nodes, num_places);
    }
    SoftXMT_signal_done();

}


