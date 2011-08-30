#include <stdlib.h>
#include <stdio.h>

#include <gasnet.h>
#include "getput.h"
#include "global_array.h"
#include "global_memory.h"
#include "tasklib.h"

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

typedef uint64_t TreeNode_ptr; //(global_array index)

#define MAX_NUM_CORES 12
gasnet_hsl_lock steal_locks[MAX_NUM_CORES];
TreeNode_ptr steal_boxes[MAX_NUM_CORES];
bool steal_full[MAX_NUM_CORES];

#define TREENODE_NULL UINT64_MAX

#define ExploreTree(root) { \
    TreeNode_ptr rr = (root); \
    if (rr!=TREENODE_NULL) { \
        TreeNode rn; \
        mem_tag_t tag = get_double_long_nb(ibt, rr, &rn); \
        yield(); \
        if (tag.dest) { \
            rn = *((TreeNode*)tag.src); \
        } else { \
            gasnet_handle_t h = (gasnet_handle_t) tag.src; \
            while (GASNET_OK != gasnet_try_syncnb(h)) { \
                yield(); \
            } \
        } \
        spawnET(rn.left, &wsqs[core_index]); \
        spawnET(rn.right, &wsqs[core_index]); \
    } \
} 

template class CircularArray<TreeNode_ptr>;
template class CircularWSDeque<TreeNode_ptr>;


typedef struct worker_data {
    int index; 
} worker_data;


void spawnET(TreeNode_ptr t, CicularWSDeque<TreeNode_ptr>* my_wsq) {
    my_wsq->pushBottom(t);
}

void run(CircularWSDeque<TreeNode_ptr>* wsqs, int core_index, int num_threads) {
    task threads[num_threads];
    worker_data datas[num_threads];
    
    task_scheduler(threads, datas, num_threads, worker_data) {
        current_data->index = current_scheduler->current_task;
        while (1) {
            TreeNode_ptr w;

            bool gotWork = false; 
            /** get work **/
            {

            // get work "arguments"
            CircularWSDeque<TreeNode_ptr>* qs = &wsqs;
            int myIndex = coreIndex;
            TreeNode_ptr* ele = &w;
    
            // get work "body"
            TreeNode_ptr res;
            if (CWSD_OK == qs[myIndex].popBottom(&res)) {
                *ele = res;
                gotWork = true;  /* return */
            } else { // try to steal
                int i=myIndex;
                while(i==myIndex) i=random()%num_threads; //XXX num threads

                int victim_node = i / num_nodes;
                if (victim_node==myNode) {             // TODO hierarchical heuristic to try stealing local first
                    // steal from a local queue
                    if (CWSD_OK == qs[i].steal(&res)) {
                        *ele = res;
                        gotWork = true; /* return */
                    } else {
                        gotWork = false; /* return */
                    }
                } else {
                    gasnet_hsl_t* lock = &steal_locks[myIndex];

                    gasnet_hsl_lock(lock); // TODO: volatile and compiler mem barrier
                    steal_full[myIndex] = false;
                    gasnet_hsl_unlock(lock);

                    gasnet_AMRequestShort2 (victim_node, WORKSTEAL_REQUEST_HANDLER, i % num_nodes, myIndex);
                    yield();
                    while (true) {
                        gasnet_hsl_lock(lock);
                        if (steal_full[myIndex]) {
                            res = steal_boxes[myIndex];
                            gasnet_hsl_unlock(lock);

                            if (TREENODE_NULL==res) { // null indicates failure to steal
                                gotWork = false; /* return */
                                break;
                            }

                            *ele = res;
                            gotWork = true; /* return */
                            break;
                        } else {
                            // wait for reply
                            gasnet_hsl_unlock(lock);
                            yield();
                        }
                    }
                }
            }

            }
            /** end get work **/

            if (gotWork) {
                ExploreTree(w);
            } else {
                yield();
                continue; // TODO figure our termination
            }
        }
    } end_scheduler();
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
        void Print() {
            PrintTree(ibt, 0);
        }
        void Explore() {
            ExploreTree(0); //root = index 0
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
        void PrintTree(TreeNode_ptr root, int depth) {
            if (root) {
                PrintTree(root -> right, depth + 1);
                for (int i = 0; i < depth; i++) printf(" "); printf("O\n");
                PrintTree(root -> left, depth + 1);
            }
        }

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

void usage() {
    fprintf(stderr, "\nusage:\n\ttree <size > 0> <imbalance > 0> \ncreates binary tree with <size> > 0 nodes each internal node having one subtree with 1/<imbalance> of the descendents.\nFor example\n\ttree 100 2\ncreates a balanced binary tree with 100 nodes.\n\ttree 100 3\ncreates a tree in which each subtree has about either half or twice the nodes of its sibling, ensuring moderate imbalance.\n");
    exit(0);
}

main(int argc, char * argv[]) {
    int size, imbalance;
    if (argc != 3) usage();
    if ((size = atoi(&argv[1][0])) < 1) usage();
    if ((imbalance = atoi(&argv[2][0])) < 1) usage();

    for (int core=0; core<num_cores; core++) {
        gasnet_hsl_init(&steal_locks[core]);
    }

    WAIT_BARRIER;
    IBT * t = new IBT(size, imbalance);
    /*   t -> Print(); */
    WAIT_BARRIER;

    int rank = gasnet_mynode();
    int num_nodes = gasnet_nodes();

    CircularWSDeque<TreeNode_ptr> local_wsqs [num_cores];

    // put root of tree in queue of first core of first rank
    if (0 == rank) local_wsqs[0].pushBottom(root);

    WAIT_BARRIER;

    printf("starting traversal\n");
    #pragma omp parallel for num_threads(num_cores)
    for (int core=0; core<num_cores; core++) {
        run(&wsqs, core, num_threads_per_core);
    }




    for (int core=0; core<num_cores; core++) {
        gasnet_hsl_destroy(&steal_locks[core]);
    }
}

#define STEAL_ABORT_RETRIES 1
void workStealRequestHandler(gasnet_token_t token, gasnet_handlerarg_t a0, gasnet_handlerarg_t a1) {
    uint64_t local_index = (uint64_t) a0;
    uint64_t coreid = (uint64_t) a1;

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

    gasnet_AMReplyShort2 (token, WORKSTEAL_REPLY_HANDLER, t, coreid);
}

void workStealReplyHandler(gasnet_token_t token, gasnet_handlerarg_t a0) {
    TreeNode_ptr t = (TreeNode_ptr) a0;
    uint64_t coreid = (uint64_t) a1;

    gasnet_hsl_lock(steal_locks[coreid]);
    steal_boxes[coreid] = t;
    steal_full[coreid] = true;
    gasnet_hsl_unlock(steal_locks[coreid]);
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
