

#define SHARED_INDEF /*nothing*/

#define GASNET_LOCKS 1

#if GASNET_LOCKS
    #include <gasnet.h>
    #define INIT_LOCK gasnet_hsl_init
    #define SET_LOCK gasnet_hsl_lock
    #define UNSET_LOCK gasnet_hsl_unlock
    #define LOCK_T gasnet_hsl_t
#endif

struct node_t {
  int type;          // distribution governing number of children
  int height;        // depth of this node in the tree
  int numChildren;   // number of children, -1 => not yet determined
};

typedef struct node_t Node;

/* Tree type
 *   Trees are generated using a Galton-Watson process, in 
 *   which the branching factor of each node is a random 
 *   variable.
 *   
 *   The random variable can follow a binomial distribution
 *   or a geometric distribution.  Hybrid tree are
 *   generated with geometric distributions near the
 *   root and binomial distributions towards the leaves.
 */
enum   uts_trees_e    { BIN = 0, GEO, HYBRID, BALANCED };
enum   uts_geoshape_e { LINEAR = 0, EXPDEC, CYCLIC, FIXED };

typedef enum uts_trees_e    tree_t;
typedef enum uts_geoshape_e geoshape_t;


//unused
#define SS_NSTATES 1   


/* stack of nodes */
struct stealStack_t
{
  int stackSize;     /* total space avail (in number of elements) */
  int workAvail;     /* elements available for stealing */
  int sharedStart;   /* index of start of shared portion of stack */
  int local;         /* index of start of local portion */
  int top;           /* index of stack top */
  int maxStackDepth;                      /* stack stats */ 
  int nNodes, maxTreeDepth;               /* tree stats  */
  int nLeaves;
  int nAcquire, nRelease, nSteal, nFail;  /* steal stats */
  int wakeups, falseWakeups, nNodes_last;
  double time[SS_NSTATES], timeLast;         /* perf measurements */
  int entries[SS_NSTATES], curState; 
  LOCK_T * stackLock; /* lock for manipulation of shared portion */
  Node * stack;       /* addr of actual stack of nodes in local addr space */
  SHARED_INDEF Node * stack_g; /* addr of same stack in global addr space */

};
typedef struct stealStack_t StealStack;



void ss_mkEmpty(StealStack *s);
void ss_error(char *str);
void ss_init(StealStack *s, int nelts);
void ss_push(StealStack *s, Node *c);
Node * ss_top(StealStack *s);
void ss_pop(StealStack *s);
int ss_topPosn(StealStack *s);
int ss_localDepth(StealStack *s);
void ss_release(StealStack *s, int k);
int ss_acquire(StealStack *s, int k);
int ss_steal_locally(int thief_id, int k, StealStack* stealStacks, int numQueues);
int ss_remote_steal(StealStack *s, int thiefCore, int victimNode, int k);
int ss_setState(StealStack *s, int state);

#define MAX_NUM_THREADS 12
#define MAXSTACKDEPTH 500000
extern StealStack stealStacks[MAX_NUM_THREADS];
