/*   $Source: /var/local/cvs/gasnet/gasnet_pshm.c,v $
 *     $Date: 2012/01/09 22:00:57 $
 * $Revision: 1.47 $
 * Description: GASNet infrastructure for shared memory communications
 * Copyright 2009, E. O. Lawrence Berekely National Laboratory
 * Terms of use are as specified in license.txt
 */

#include <gasnet_internal.h>

#if GASNET_PSHM /* Otherwise file is empty */

#include <gasnet_core_internal.h> /* for gasnetc_{Short,Medium,Long} and gasnetc_handler[] */

#include <sys/types.h>
#include <signal.h>

#if defined(GASNETI_USE_GENERIC_ATOMICOPS) || defined(GASNETI_USE_OS_ATOMICOPS)
  #error "GASNet PSHM support requires Native atomics"
#endif

/* max # of incoming requests per node, per supernode peer */
#define GASNETI_PSHMNET_DEFAULT_QUEUE_DEPTH 8
#define GASNETI_PSHMNET_MAX_QUEUE_DEPTH 1024
#define GASNETI_PSHMNET_MIN_QUEUE_DEPTH 2

/* payload memory available for outstanding requests, per node
 * default will be silently raised as needed for large node count */
#define GASNETI_PSHMNET_DEFAULT_QUEUE_MEMORY (1<<20)
#define GASNETI_PSHMNET_MAX_QUEUE_MEMORY (1<<28) 

/* Global vars */
gasneti_pshmnet_t *gasneti_request_pshmnet = NULL;
gasneti_pshmnet_t *gasneti_reply_pshmnet = NULL;
 
#ifndef GASNET_PSHM_FULLEMPTY
#define GASNET_PSHM_FULLEMPTY 1
#endif
#ifndef GASNET_PSHM_FULLEMPTY_USE_SUB
#define GASNET_PSHM_FULLEMPTY_USE_SUB 1
#endif
#ifndef GASNET_PSHM_FULLEMPTY_USE_DEC
#define GASNET_PSHM_FULLEMPTY_USE_DEC 1
#endif
#if GASNET_PSHM_FULLEMPTY
  typedef struct {
    gasneti_atomic_t value; /* Might be a boolean or counter */
    char _pad[GASNETI_CACHE_PAD(sizeof(gasneti_atomic_t))];
  } gasneti_pshmnet_fullempty_t;
  #define _GASNETI_PSHMNET_FE(vnet,node)	(&((vnet)->fullempty[(node)].value))
  #define GASNETI_PSHMNET_FE_READ(vnet,node) \
		gasneti_atomic_read(_GASNETI_PSHMNET_FE((vnet),(node)), 0)
  #define GASNETI_PSHMNET_FE_INIT(vnet,node) \
		gasneti_atomic_set(_GASNETI_PSHMNET_FE((vnet),(node)), 0, 0)
  #if defined(GASNETI_HAVE_ATOMIC_ADD_SUB) && GASNET_PSHM_FULLEMPTY_USE_SUB
    /* inc/sub of counter 'value' */
    #define GASNETI_PSHMNET_FE_INC(vnet,node) \
		gasneti_atomic_increment(_GASNETI_PSHMNET_FE((vnet),(node)), 0)
    #define GASNETI_PSHMNET_FE_DEC(vnet,node)		((void)0)
    #define GASNETI_PSHMNET_FE_SUB(vnet,node,val)	\
	do { \
		int _val = (val); \
		if (_val) gasneti_atomic_subtract(_GASNETI_PSHMNET_FE((vnet),(node)), (_val), 0); \
	} while (0)
    #define GASNETI_PSHMNET_FE_ACK(vnet,node)		((void)0)
  #elif GASNET_PSHM_FULLEMPTY_USE_DEC
    /* inc/dec of counter 'value' */
    #define GASNETI_PSHMNET_FE_INC(vnet,node) \
		gasneti_atomic_increment(_GASNETI_PSHMNET_FE((vnet),(node)), 0)
    #define GASNETI_PSHMNET_FE_DEC(vnet,node) \
		gasneti_atomic_decrement(_GASNETI_PSHMNET_FE((vnet),(node)), 0)
    #define GASNETI_PSHMNET_FE_SUB(vnet,node,val)	((void)0)
    #define GASNETI_PSHMNET_FE_ACK(vnet,node)		((void)0)
  #else
    /* set/clear of boolean 'value' */
    #define GASNETI_PSHMNET_FE_INC(vnet,node) \
		gasneti_atomic_set(_GASNETI_PSHMNET_FE((vnet),(node)), 1, 0)
    #define GASNETI_PSHMNET_FE_DEC(vnet,node)		((void)0)
    #define GASNETI_PSHMNET_FE_SUB(vnet,node,val)	((void)0)
    #define GASNETI_PSHMNET_FE_ACK(vnet,node) \
		gasneti_atomic_set(_GASNETI_PSHMNET_FE((vnet),(node)), 0, 0)
  #endif
#else
  #define GASNETI_PSHMNET_FE_READ(vnet,node)	1
  #define GASNETI_PSHMNET_FE_INIT(vnet,node)	((void)0)
  #define GASNETI_PSHMNET_FE_INC(vnet,node)	((void)0)
  #define GASNETI_PSHMNET_FE_DEC(vnet,node)	((void)0)
  #define GASNETI_PSHMNET_FE_SUB(vnet,node,val)	((void)0)
  #define GASNETI_PSHMNET_FE_ACK(vnet,node)	((void)0)
#endif

/* Structure for PSHM intra-supernode barrier */
gasneti_pshm_barrier_t *gasneti_pshm_barrier = NULL; /* lives in shared space */

static int gasneti_pshmnet_queue_depth = 0;
static uintptr_t gasneti_pshmnet_queue_mem = 0;

static void *gasnetc_pshmnet_region = NULL;

static struct gasneti_pshm_info {
    gasneti_atomic_t    bootstrap_barrier;
    /* early_barrier will be overwritten by other vars after its completion */
    /* sig_atomic_t should be wide enough to avoid word-tearing, right? */
    volatile sig_atomic_t early_barrier[1]; /* variable length array */
} *gasneti_pshm_info = NULL;
#define GASNETI_PSHM_BSB_LIMIT (GASNETI_ATOMIC_MAX - gasneti_pshm_nodes)

#define round_up_to_pshmpage(size_or_addr)               \
        GASNETI_ALIGNUP(size_or_addr, GASNETI_PSHMNET_PAGESIZE)

#define pshmnet_get_struct_addr_from_field_addr(structname, fieldname, fieldaddr) \
        ((structname*)(((uintptr_t)fieldaddr) - offsetof(structname,fieldname)))

void *gasneti_pshm_init(gasneti_bootstrapExchangefn_t exchangefn, size_t aux_sz) {
  size_t vnetsz, mmapsz;
  int discontig = 0;
  gasneti_pshm_rank_t *pshm_max_nodes;
  gasnet_node_t i;
#if !GASNET_CONDUIT_SMP
  gasnet_node_t j;
#endif

  gasneti_assert(exchangefn != NULL);  /* NULL exchangefn no longer supported */

  /* Testing if the number of PSHM nodes is always smaller than GASNETI_PSHM_MAX_NODES */
  pshm_max_nodes = gasneti_calloc(gasneti_nodes, sizeof(gasneti_pshm_rank_t));
  for(i=0; i<gasneti_nodes; i++){
    /* Note combination of post-increment and == guard against overflow */
    if ((pshm_max_nodes[gasneti_nodemap[i]]++) == GASNETI_PSHM_MAX_NODES){
      if (gasneti_mynode == gasneti_nodemap[i]){
        gasneti_fatalerror("PSHM nodes requested on node '%s' exceeds maximum (%d)\n", 
                        gasneti_gethostname(), GASNETI_PSHM_MAX_NODES);
      } else {
        gasneti_fatalerror("PSHM nodes requested on some node exceeds maximum (%d)\n", 
                        GASNETI_PSHM_MAX_NODES);
      }
    }
  }
  gasneti_free(pshm_max_nodes);
    
  gasneti_pshm_nodes = gasneti_nodemap_local_count;
  gasneti_pshm_firstnode = gasneti_nodemap[gasneti_mynode];
  gasneti_pshm_mynode = gasneti_nodemap_local_rank;

#if GASNET_CONDUIT_SMP
  gasneti_assert(gasneti_pshm_nodes == gasneti_nodes);
  gasneti_assert(gasneti_pshm_firstnode == 0);
  gasneti_assert(gasneti_pshm_mynode == gasneti_mynode);
#else
  /* TODO: Allow env var to limit size of a supernode. */
  gasneti_assert(gasneti_nodemap[0] == 0);
  for (i=1; i<gasneti_nodes; ++i) {
    /* Determine if supernode members are numbered contiguously */
    if (gasneti_nodemap[i-1] > gasneti_nodemap[i]) {
      discontig = 1;
      break;
    }
  }
#endif

  gasneti_assert(gasneti_pshm_supernodes > 0);

  /* compute size of vnet shared memory region */
  vnetsz = gasneti_pshmnet_memory_needed(gasneti_pshm_nodes); 
  mmapsz = (2*vnetsz);
  { /* gasneti_pshm_info contains multiple variable-length arrays in the same space */
    size_t info_sz;
    /* space for gasneti_pshm_firsts: */
    info_sz = gasneti_pshm_supernodes * sizeof(gasnet_node_t);
    /* optional space for gasneti_pshm_rankmap: */
    if (discontig) {
      info_sz = GASNETI_ALIGNUP(info_sz, sizeof(gasneti_pshm_rank_t));
      info_sz += gasneti_nodes * sizeof(gasneti_pshm_rank_t);
    }
    /* space for the PSHM intra-node barrier: */
    info_sz = GASNETI_ALIGNUP(info_sz, GASNETI_CACHE_LINE_BYTES);
    info_sz += sizeof(gasneti_pshm_barrier_t) +
	       (gasneti_pshm_nodes-1) * sizeof(gasneti_pshm_barrier->node);
    /* space for early barrier, sharing space with the items above: */
    info_sz = MAX(info_sz, gasneti_pshm_nodes * sizeof(sig_atomic_t));
    info_sz += offsetof(struct gasneti_pshm_info, early_barrier);
    /* final space requested: */
    mmapsz += round_up_to_pshmpage(info_sz);
  }
  mmapsz += round_up_to_pshmpage(aux_sz);

  /* setup vnet shared memory region for AM infrastructure and supernode barrier.
   */
  gasnetc_pshmnet_region = gasneti_mmap_vnet(mmapsz, exchangefn);
  if (gasnetc_pshmnet_region == NULL) {
    gasneti_unlink_vnet();
    gasneti_fatalerror("Failed to mmap %lu bytes for shared memory Active Messages region.",
                       (unsigned long)mmapsz);
  }
  
  /* Prepare the shared info struct (including bootstrap barrier) */
  gasneti_pshm_info = (struct gasneti_pshm_info *)((uintptr_t)gasnetc_pshmnet_region + 2*vnetsz);
  if (gasneti_pshm_mynode != 0) {
    /* For a few architectures we cannot assume that the pre-zeroed memory we
     * receive will correspond to an atomic counter of value zero. */
    gasneti_atomic_set(&gasneti_pshm_info->bootstrap_barrier, 0, 0);
  }
  gasneti_pshm_info->early_barrier[gasneti_pshm_mynode] = 1;
  gasneti_local_wmb();

  /* "early" barrier which protects initialization of the real barrier counter. */
  for (i=0; i < gasneti_pshm_nodes; ++i) {
    gasneti_waituntil(gasneti_pshm_info->early_barrier[i] != 0);
  }

  /* Unlink the shared memory file to prevent leaks.
   * "early" barrier above ensures all procs have attached. */
  gasneti_unlink_vnet();

  /* Carve out various allocations, reusing the "early barrier" space. */
  gasneti_pshmnet_bootstrapBarrier();
  {
    uintptr_t addr = (uintptr_t)&gasneti_pshm_info->early_barrier;
    /* gasneti_pshm_firsts, an array of gasneti_pshm_supernodes*sizeof(gasnet_node_t): */
    gasneti_pshm_firsts = (gasnet_node_t *)addr;
    addr += gasneti_pshm_supernodes * sizeof(gasnet_node_t);
    /* optional rankmap: */
    if (discontig) {
      addr = GASNETI_ALIGNUP(addr, sizeof(gasneti_pshm_rank_t));
      gasneti_pshm_rankmap = (gasneti_pshm_rank_t *)addr;
      addr += gasneti_nodes * sizeof(gasneti_pshm_rank_t);
    }
    /* intra-supernode barrier: */
    addr = GASNETI_ALIGNUP(addr, GASNETI_CACHE_LINE_BYTES);
    gasneti_pshm_barrier = (gasneti_pshm_barrier_t *)addr;
    addr += sizeof(gasneti_pshm_barrier_t) +
	    (gasneti_pshm_nodes-1) * sizeof(gasneti_pshm_barrier->node);
  }

  /* Populate gasneti_pshm_firsts[] */
  if (!gasneti_pshm_mynode) gasneti_pshm_firsts[0] = 0;
#if !GASNET_CONDUIT_SMP
  for (i=j=1; i<gasneti_nodes; ++i) {
    if (gasneti_nodemap[i] == i) {
      if (!gasneti_pshm_mynode) gasneti_pshm_firsts[j] = i;
      j += 1;
      gasneti_assert(j <= gasneti_pshm_supernodes);
    }
  }
#endif
#if GASNET_DEBUG
  /* Validate gasneti_pshm_firsts[] */
  if (gasneti_pshm_mynode == 0) {
    for (i=0; i<gasneti_pshm_supernodes; ++i) {
      gasneti_assert(!i || (gasneti_pshm_firsts[i] > gasneti_pshm_firsts[i-1]));
      gasneti_assert(gasneti_nodemap[gasneti_pshm_firsts[i]] == gasneti_pshm_firsts[i]);
    }
  }
#endif

  /* construct rankmap, if any, with first node doing all the work. */
  if (discontig) {
    if (gasneti_pshm_mynode == 0) { /* First node does all the work */
      memset(gasneti_pshm_rankmap, 0xff, gasneti_nodes * sizeof(gasneti_pshm_rank_t));
      for (i = 0; i < gasneti_pshm_nodes; ++i) {
        gasneti_pshm_rankmap[gasneti_nodemap_local[i]] = i;
      }
    }
  }

  /* Collective call to initialize Shared AM "networks" */
  gasneti_request_pshmnet = 
          gasneti_pshmnet_init(gasnetc_pshmnet_region,
                               vnetsz, gasneti_pshm_nodes);
  gasneti_reply_pshmnet =
          gasneti_pshmnet_init((void*)((uintptr_t)gasnetc_pshmnet_region + vnetsz),
                               vnetsz, gasneti_pshm_nodes);

  /* Ensure all peers are initialized before return */
  gasneti_pshmnet_bootstrapBarrier();

  /* Return the conduit's portion, if any */
  return aux_sz ? (void*)((uintptr_t)gasnetc_pshmnet_region +
                          mmapsz - round_up_to_pshmpage(aux_sz))
                : NULL;
}

/* Defaults if gasnet_core_fwd.h doesn't #define these preprocessor tokens */
#ifndef GASNETC_MAX_ARGS_PSHM
  /* Assumes gasnet_AMMaxArgs() expands to a compile-time constant */
  #define GASNETC_MAX_ARGS_PSHM   (gasnet_AMMaxArgs())
#endif
#ifndef GASNETC_MAX_MEDIUM_PSHM
  /* Assumes gasnet_AMMaxMedium() expands to a compile-time constant */
  #define GASNETC_MAX_MEDIUM_PSHM (gasnet_AMMaxMedium())
#endif
#ifndef GASNETC_GET_HANDLER
  /* Assumes conduit has gasnetc_handler[] as in template-conduit */
  #define gasnetc_get_handler(_h) (gasnetc_handler[(_h)])
#endif
#ifndef GASNETC_TOKEN_CREATE
  /* Our default implementation is suitable for conduits that use a pointer
   * for gasnet_token_t.  We generate tokens with the least-significant bit
   * set.  This distinguishes them from a valid pointer, but still allows a
   * (token != NULL) assertion.
   */
  #if GASNET_DEBUG
    typedef struct {
      gasnet_node_t srcNode;
      int isReq;
      int replySent;
    } gasneti_ampshm_token_t;

    static gasnet_token_t gasnetc_token_create(gasnet_node_t src, int isReq) {
      gasneti_ampshm_token_t *my_token = gasneti_malloc(sizeof(gasneti_ampshm_token_t));
      gasneti_assert(!((uintptr_t)my_token & 1));
      gasneti_assert(gasneti_pshm_in_supernode(src));
      my_token->srcNode = src;
      my_token->isReq = isReq;
      my_token->replySent = 0;
      return (gasnet_token_t)(1|(uintptr_t)my_token);
    }

    #define gasnetc_token_destroy(tok) gasneti_free((void*)(1^(uintptr_t)(tok)))

    extern void gasnetc_token_reply(gasnet_token_t token) {
      gasneti_ampshm_token_t *my_token = (gasneti_ampshm_token_t *)(1^(uintptr_t)token);
      gasneti_assert(gasnetc_token_is_pshm(token));
      gasneti_assert(my_token);
      gasneti_assert(my_token->isReq);
      gasneti_assert(!my_token->replySent);
      my_token->replySent = 1;
    }

    extern int gasneti_AMPSHMGetMsgSource(gasnet_token_t token, gasnet_node_t *src_ptr) {
      int retval = GASNET_ERR_BAD_ARG;
      if (gasnetc_token_is_pshm(token)) {
        gasneti_ampshm_token_t *my_token = (gasneti_ampshm_token_t *)(1^(uintptr_t)token);
        gasnet_node_t tmp = my_token->srcNode;
        gasneti_assert(gasneti_pshm_in_supernode(tmp));
        *src_ptr = tmp;
        retval = GASNET_OK;
      }
      return retval;
    }
  #else
    /* We encode the source in a uintptr_t but ignore isReq.  */
    #define gasnetc_token_create(_src, _isReq) \
      ((gasnet_token_t)(1 | ((uintptr_t)(_src) << 1)))
    #define gasnetc_token_destroy(tok) ((void)0)
  #endif
#endif


/*******************************************************************************
 * PSHM global variables:
 ******************************************************************************/
gasneti_pshm_rank_t gasneti_pshm_nodes = 0;
gasnet_node_t gasneti_pshm_firstnode = (gasnet_node_t)(-1);
gasneti_pshm_rank_t gasneti_pshm_mynode = (gasneti_pshm_rank_t)(-1);
/* vectors constructed in shared space: */
gasneti_pshm_rank_t *gasneti_pshm_rankmap = NULL;
gasnet_node_t *gasneti_pshm_firsts = NULL;

/*******************************************************************************
 * "PSHM Net":  message header formats
 * These come early because their sizes influence allocation
 ******************************************************************************/

/* TODO: Could/should we squeeze unused args out of a Medium.*/
/* TODO: Pack category and numargs together (makes assumtion about ranges) */

typedef struct {
  uint8_t category;      /* AM msg type: short, med, long */
  uint8_t numargs;
  gasnetc_handler_t handler_id;
  gasnet_node_t source;
  gasnet_handlerarg_t args[GASNETC_MAX_ARGS_PSHM];
} gasneti_AMPSHM_msg_t;
typedef gasneti_AMPSHM_msg_t gasneti_AMPSHM_shortmsg_t;

typedef struct {
  gasneti_AMPSHM_msg_t msg;
#if (GASNETI_MAX_MEDIUM_PSHM < 65536) /* GASNET<C>_MAX_MEDIUM_PSHM often not a preprocess-time constant */
  uint16_t numbytes;
#else
  uint32_t numbytes;
#endif
  uint8_t  mediumdata[6 + GASNETC_MAX_MEDIUM_PSHM]; /* Is 2, 4 or 8-byte aligned */
} gasneti_AMPSHM_medmsg_t;

typedef struct {
  gasneti_AMPSHM_msg_t msg;
  uint32_t numbytes;
  void *   longdata;
} gasneti_AMPSHM_longmsg_t;

typedef union {
  gasneti_AMPSHM_shortmsg_t Short;
  gasneti_AMPSHM_medmsg_t   Medium;
  gasneti_AMPSHM_longmsg_t  Long;
} gasneti_AMPSHM_maxmsg_t;

/* Values for gasneti_pshmnet_msg_t.state */
enum {
  GASNETI_PSHMNET_EMPTY = 0,
  GASNETI_PSHMNET_FULL,
  GASNETI_PSHMNET_BUSY
};

/* state of message queue entry */
#if defined(GASNETI_HAVE_ATOMIC_CAS)
  typedef gasneti_atomic_t gasneti_pshmnet_msg_state_t;
  #define gasneti_pshmnet_state_init(_s)\
	          gasneti_atomic_set((_s), GASNETI_PSHMNET_EMPTY, 0)
  #define gasneti_pshmnet_state_set(_s,_v,_f)\
                  gasneti_atomic_set((_s),(_v),(_f))
  #define gasneti_pshmnet_state_swap(_s,_o,_n,_f)\
                  gasneti_atomic_compare_and_swap((_s),(_o),(_n),(_f))
  #define gasneti_pshmnet_state_read(_s)\
                  gasneti_atomic_read((_s),0)
#elif defined(GASNETI_HAVE_ATOMIC_ADD_SUB)
  typedef struct gasneti_pshmnet_msg_state_ {
    gasneti_atomic_t last_ticket;
    gasneti_atomic_t curr_ticket;
    int value;
  } gasneti_pshmnet_msg_state_t;

  GASNETI_INLINE(gasneti_pshmnet_state_init)
  void gasneti_pshmnet_state_init(gasneti_pshmnet_msg_state_t *s) {
    s->value = GASNETI_PSHMNET_EMPTY;
    gasneti_atomic_set(&s->last_ticket, 0, 0);
    gasneti_atomic_set(&s->curr_ticket, 1, 0);
  }
  GASNETI_INLINE(gasneti_pshmnet_state_lock)
  void gasneti_pshmnet_state_lock(gasneti_pshmnet_msg_state_t *s) {
    const gasneti_atomic_val_t my_ticket = gasneti_atomic_add(&s->last_ticket, 1, 0);
    gasneti_waituntil(my_ticket == gasneti_atomic_read(&s->curr_ticket, 0)); /* includes RMB */
  }
  GASNETI_INLINE(gasneti_pshmnet_state_unlock)
  void gasneti_pshmnet_state_unlock(gasneti_pshmnet_msg_state_t *s) {
    gasneti_atomic_increment(&s->curr_ticket, GASNETI_ATOMIC_REL);
  }
  GASNETI_INLINE(gasneti_pshmnet_state_set)
  void gasneti_pshmnet_state_set(gasneti_pshmnet_msg_state_t *s, int value, int fence) {
    /* ignore "fence" since lock+unlock is an unconditional full MB */
    gasneti_pshmnet_state_lock(s);
    s->value = value;
    gasneti_pshmnet_state_unlock(s);
  }
  GASNETI_INLINE(gasneti_pshmnet_state_swap)
  int gasneti_pshmnet_state_swap(gasneti_pshmnet_msg_state_t *s, int oldval, int newval, int fence) {
    /* ignore "fence" since lock+unlock is an unconditional full MB */
    int result;
    gasneti_pshmnet_state_lock(s);
    if_pt (result = (s->value == oldval))
      s->value = newval;
    gasneti_pshmnet_state_unlock(s);
    return result;
  }
  #define gasneti_pshmnet_state_read(_s)\
                  ((_s)->value)
#else
  #error "Platform is missing both atomic ADD and atomic CAS"
#endif

/* data about an incoming message */
typedef struct gasneti_pshmnet_msg {
  void * addr;
  size_t len;
  unsigned int next_idx;
  gasneti_pshmnet_msg_state_t state;
  /* Paul informs me that padding with GASNETI_CACHE_PAD ensures the struct
   * is sizeof(cache_line), but not that it's aligned on a single cache line.
   * But we enforce cache line alignment, so we're OK */
  #if 1
    char _pad[GASNETI_CACHE_PAD(sizeof(void *)
                               +sizeof(size_t)
                               +sizeof(int)
                               +sizeof(gasneti_pshmnet_msg_state_t))];
  #else
   /* Alternative: pad out the struct to two cache lines, to ensure we'll have
    * no spurious cache line conflicts.  Many vapi structs use this too. */
    char _pad[GASNETI_CACHE_LINE_BYTES];
  #endif
} gasneti_pshmnet_msg_t;

/* Used in place of NULL, since zero-offset is legal */
#define GASNETI_PSHMNET_MSG_NULL ((gasneti_pshmnet_msg_t *)(~(uintptr_t)0))

/* Circular queue of info about messages received or sent */
typedef struct gasneti_pshmnet_queue {
  /* Only need to lock queue ptr if client multithreaded */
  gasneti_mutex_t lock;
  gasneti_pshmnet_msg_t *next;
  gasneti_pshmnet_msg_t *queue;
  #if 1
    /* See above comment about cache alignment.  Not that we might not
     * get perfect cache line alignment, since internal padding might
     * cause us to add too much (but never too little) padding here */
    char _pad[GASNETI_CACHE_PAD(sizeof(void *)*2
                               +sizeof(gasneti_mutex_t))];
  #else
    char _pad[GASNETI_CACHE_LINE_BYTES];
  #endif
} gasneti_pshmnet_queue_t;

struct gasneti_pshmnet_allocator;  /* forward definition */

/* message payload metadata */
typedef struct gasneti_pshmnet_payload {
  gasneti_pshmnet_msg_t *msg;
  struct gasneti_pshmnet_allocator *allocator;
  gasneti_AMPSHM_maxmsg_t data;
} gasneti_pshmnet_payload_t;

/******************************************************************************
 * Payload memory allocator interface.
 *
 * Keep the payload allocator interface clean, so we can change algorithms.
 ******************************************************************************/

/* Memory layout of allocator.
 *
 * Logically, we have this layout:
 *  
 *    1) Atomic used by allocator as 'in use' bit
 *    2) A gasneti_pshmnet_payload_t, used by pshmnet
 *
 * Allocator returns #2 to the caller
 */ 

typedef struct {
  gasneti_atomic_t in_use;
  gasneti_pshmnet_payload_t payload;
} gasneti_pshmnet_allocator_block_t;

#define GASNETI_PSHMNET_ALLOC_MAXSZ \
    GASNETI_ALIGNUP(sizeof(gasneti_pshmnet_allocator_block_t), GASNETI_PSHMNET_PAGESIZE)
#define GASNETI_PSHMNET_ALLOC_MAXPG (GASNETI_PSHMNET_ALLOC_MAXSZ >> GASNETI_PSHMNET_PAGESHIFT)

#define GASNETI_PSHMNET_MAX_PAYLOAD \
    (GASNETI_PSHMNET_ALLOC_MAXSZ - offsetof(gasneti_pshmnet_allocator_block_t, payload.data))

size_t gasneti_pshmnet_max_payload(void) {
  return GASNETI_PSHMNET_MAX_PAYLOAD;
}

/* This implementation uses a circular queue of variable-sized payloads.
 * This data structure lives entirely in private memory.
 *
 * The allocator's per-block metadata is external to the blocks, and is
 * stored in the "length" array.  There is a length entry for each "page",
 * but only the entries corresponding to the first page of each block have
 * defined values.  The "length[]", "next" and "count" are all in units
 * of GASNETI_PSHMNET_PAGESIZE (which must be a power-of-2).
 */
typedef struct gasneti_pshmnet_allocator {
  void *region;
  unsigned int next;
  unsigned int count;
  unsigned int length[1]; /* Variable length */
} gasneti_pshmnet_allocator_t;

/* WARNING: the amount requested from this allocator must be less than 
 * or equal to GASNETI_PSHMNET_MAX_PAYLOAD (which is at least as large
 * as sizeof(gasneti_pshmnet_payload_t)).
 * - returns NULL if no memory available
 */
static gasneti_pshmnet_allocator_t *gasneti_pshmnet_init_allocator(void *region, size_t len);
static void * gasneti_pshmnet_alloc(gasneti_pshmnet_allocator_t *a, size_t nbytes);
/* Frees memory.  Note that this must be callable by a different node */
static void gasneti_pshmnet_free(gasneti_pshmnet_allocator_t *a, void *p);

/******************************************************************************
 * </Payload memory allocator interface>
 ******************************************************************************/

/* Per node view of 'network' of queues in supernode 
 * - Note that this struct itself is not stored in shared memory. 
 */
struct gasneti_pshmnet {
  gasneti_pshm_rank_t nodecount;    /* nodes in supernode */ 
  gasneti_weakatomic_t nextindex;   /* index of next node to check for msgs */
  /* arrays of queue pointers and their locks */
  gasneti_pshmnet_queue_t *in_queues;
  gasneti_pshmnet_queue_t *out_queues;
  /* only need to see one's own allocator */
  gasneti_pshmnet_allocator_t *my_allocator;
#if GASNET_PSHM_FULLEMPTY
  /* array of full/empty bits */
  gasneti_pshmnet_fullempty_t *fullempty;
#endif
};

#define gasneti_assert_align(p, align) \
        gasneti_assert((((uintptr_t)p) % align) == 0)

/* Macros for determining the offset and the real address, used for
 * the addresses inside the sysnet region */
#define gasneti_pshm_offset(addr) \
                (void *)((uintptr_t)(addr) - (uintptr_t)gasnetc_pshmnet_region)

#define gasneti_pshm_addr(addr) \
                (void *)((uintptr_t)(addr) + (uintptr_t)gasnetc_pshmnet_region)


static int get_queue_depth(gasneti_pshm_rank_t nodes) 
{
  int val = gasneti_getenv_int_withdefault("GASNET_PSHMNET_QUEUE_DEPTH", GASNETI_PSHMNET_DEFAULT_QUEUE_DEPTH, 0);
  if (val > GASNETI_PSHMNET_MAX_QUEUE_DEPTH) {
    fprintf(stderr, "GASNET_PSHMNET_QUEUE_DEPTH (%d) larger than max: using %d\n",
            val, GASNETI_PSHMNET_MAX_QUEUE_DEPTH);
    val = GASNETI_PSHMNET_MAX_QUEUE_DEPTH;
  } else if (val < GASNETI_PSHMNET_MIN_QUEUE_DEPTH) {
    fprintf(stderr, "GASNET_PSHMNET_QUEUE_DEPTH (%d) smaller than min: using %d\n",
            val, GASNETI_PSHMNET_MIN_QUEUE_DEPTH);
    val = GASNETI_PSHMNET_MIN_QUEUE_DEPTH;
  }
  return val;
}

static uintptr_t get_queue_mem(int nodes) 
{
  /* theoretical limit = 1 max-sized send buffer per peer?
   *   We're requiring 2 per peer right now to be safe.
   * - future implementations may also need some space for allocator's metadata */
  size_t minsize = GASNETI_PSHMNET_ALLOC_MAXSZ*nodes*2;
  uintptr_t pernode = gasneti_getenv_int_withdefault("GASNET_PSHMNET_QUEUE_MEMORY", 
                    MAX(minsize, GASNETI_PSHMNET_DEFAULT_QUEUE_MEMORY), 1<<20);
  if (pernode > GASNETI_PSHMNET_MAX_QUEUE_MEMORY) {
    fprintf(stderr, "GASNET_PSHMNET_QUEUE_MEMORY (%ld) larger than max: using %ld\n",
            (long)pernode, (long)GASNETI_PSHMNET_MAX_QUEUE_MEMORY);
    pernode = GASNETI_PSHMNET_MAX_QUEUE_MEMORY;
  } else if (pernode < minsize) {
    fprintf(stderr, "GASNET_PSHMNET_QUEUE_MEMORY (%ld) smaller than min: using %ld\n",
            (long)pernode, (long)minsize);
    pernode = minsize;
  }
  gasneti_assert(pernode > 0);

  /* round up to multiple of allocator page size */
  return GASNETI_ALIGNUP(pernode, GASNETI_PSHMNET_PAGESIZE);
}

static size_t gasneti_pshmnet_memory_needed_pernode(gasneti_pshm_rank_t nodes)
{
  size_t size = 0;

  /* Message info */
  if_pf (!gasneti_pshmnet_queue_depth) {
    gasneti_pshmnet_queue_depth = get_queue_depth(nodes);
  }
  size += sizeof(gasneti_pshmnet_msg_t)*gasneti_pshmnet_queue_depth*(nodes-1);
  size = round_up_to_pshmpage(size);

  /* Message payloads */
  if_pf (!gasneti_pshmnet_queue_mem) {
    gasneti_pshmnet_queue_mem = get_queue_mem(nodes);
  }
  size += gasneti_pshmnet_queue_mem;  
  size = round_up_to_pshmpage(size);

  return size;
}

size_t gasneti_pshmnet_memory_needed(gasneti_pshm_rank_t nodes)
{
  size_t pernode = gasneti_pshmnet_memory_needed_pernode(nodes);
  size_t once = 0;
#if GASNET_PSHM_FULLEMPTY
  once = round_up_to_pshmpage(nodes * sizeof(gasneti_pshmnet_fullempty_t));
#endif
  return once + nodes * pernode;
}

static void gasneti_pshmnet_init_my_pshm(gasneti_pshmnet_t *pvnet, void *myregion, 
                                         gasneti_pshm_rank_t nodes)
{
  int i, j;
  gasneti_pshmnet_msg_t   *mymsgs;
  void *alloc_region;
  gasneti_assert_align(myregion, GASNETI_PSHMNET_PAGESIZE);

  /* NOTE: other init code relies on msgs being at start of region */
  mymsgs = (gasneti_pshmnet_msg_t *)myregion;
  for (i = 0; i < nodes; i++) {
    if (i != gasneti_pshm_mynode) {
      for (j = 0; j < gasneti_pshmnet_queue_depth; j++) {
        gasneti_pshmnet_state_init(&mymsgs[j].state);
        mymsgs[j].next_idx = (j+1) % gasneti_pshmnet_queue_depth;
      }
      mymsgs += gasneti_pshmnet_queue_depth;
    }
  }

  alloc_region = (void*) round_up_to_pshmpage(mymsgs);
  pvnet->my_allocator = 
    gasneti_pshmnet_init_allocator(alloc_region, gasneti_pshmnet_queue_mem);
  gasneti_weakatomic_set(&pvnet->nextindex, 0, 0);
}

/* Initializes the pshmnet region. Called from each node twice: 
 * to initialize pshmnet_request and pshmnet_reply */
gasneti_pshmnet_t *
gasneti_pshmnet_init(void *start, size_t nbytes, gasneti_pshm_rank_t pshmnodes)
{
  gasneti_pshmnet_t *vnet;
  gasneti_pshm_rank_t i;
  size_t szpernode, regionlen;
  size_t szonce = 0;
  void *region, *myregion;

  /* make sure that our max buffer size fits all possible AMs */
  gasneti_assert(sizeof(gasneti_AMPSHM_maxmsg_t) <= GASNETI_PSHMNET_MAX_PAYLOAD);

  region = start;
  region = (void *)round_up_to_pshmpage(region);

  regionlen = nbytes - ( ((uintptr_t)region)-((uintptr_t)start));
  szpernode = gasneti_pshmnet_memory_needed_pernode(pshmnodes);
#if GASNET_PSHM_FULLEMPTY
  szonce = sizeof(gasneti_pshmnet_fullempty_t);
#endif
  if (regionlen < (szpernode + szonce) * pshmnodes) 
    gasneti_fatalerror("Internal error: not enough memory for pshmnet: \n"
                       " given %lu effective bytes, but need %lu", 
                       (unsigned long)regionlen, (unsigned long)((szpernode + szonce) * pshmnodes));
  vnet = gasneti_malloc(sizeof(gasneti_pshmnet_t));
  vnet->nodecount = pshmnodes;
  myregion = (void *)( ((uintptr_t)region) + (szpernode*gasneti_pshm_mynode));
  /* collective call, so each process inits its own region.
   * To allow non-fixed mapping of the pshmnet memory, we initialize
   * each reqion using the offset-addresses */
  gasneti_pshmnet_init_my_pshm(vnet, myregion, pshmnodes);

#if GASNET_PSHM_FULLEMPTY
  /* initialize the full/empty bits array */
  vnet->fullempty = (gasneti_pshmnet_fullempty_t *)(((uintptr_t)region) + (szpernode*pshmnodes));
  GASNETI_PSHMNET_FE_INIT(vnet,gasneti_pshm_mynode);
#endif

  /* initialize queue pointers */
  vnet->in_queues = gasneti_malloc(2*sizeof(gasneti_pshmnet_queue_t)*pshmnodes);
  vnet->out_queues =  vnet->in_queues + pshmnodes;
  for (i = 0; i < pshmnodes; i++) {
    if (i == gasneti_pshm_mynode) {
#if GASNET_DEBUG
      memset(&vnet->in_queues[i], 0xff, sizeof(gasneti_pshmnet_queue_t));
      memset(&vnet->out_queues[i], 0xff, sizeof(gasneti_pshmnet_queue_t));
#endif
    } else {
      /* Locations of message queue regions */
      gasneti_pshmnet_msg_t *in_msgs = (gasneti_pshmnet_msg_t *)myregion;
      gasneti_pshmnet_msg_t *out_msgs = (gasneti_pshmnet_msg_t *)((uintptr_t)region + (szpernode*i));
      /* Addresses of proper portions of queue regions */
      const int more = (i > gasneti_pshm_mynode);
      gasneti_pshmnet_msg_t *in_queue = in_msgs + gasneti_pshmnet_queue_depth * (i - more);
      gasneti_pshmnet_msg_t *out_queue = out_msgs + gasneti_pshmnet_queue_depth * (gasneti_pshm_mynode - !more);

      gasneti_mutex_init(&vnet->in_queues[i].lock);
      vnet->in_queues[i].next = vnet->in_queues[i].queue = in_queue;

      gasneti_mutex_init(&vnet->out_queues[i].lock);
      vnet->out_queues[i].next = vnet->out_queues[i].queue = out_queue;
    }
  }

  gasneti_leak(vnet);
  gasneti_leak(vnet->in_queues);

  return vnet;
}

/* Return the next message if it is in the desired state, otherwise NULL.
   Atomically changes state of the acquired message to BUSY */
GASNETI_INLINE(gasneti_pshmnet_next_msg)
gasneti_pshmnet_msg_t *gasneti_pshmnet_next_msg(gasneti_pshmnet_queue_t * const q, int state, int fence)
{
  gasneti_pshmnet_msg_t *msg;

  gasneti_mutex_lock(&q->lock);
  msg = q->next;
  if (gasneti_pshmnet_state_swap(&msg->state, state, GASNETI_PSHMNET_BUSY, fence)) {
    q->next = q->queue + msg->next_idx;
  } else {
    msg = NULL;
  }
  gasneti_mutex_unlock(&q->lock);

  return msg;
}

void * gasneti_pshmnet_get_send_buffer(gasneti_pshmnet_t *vnet, size_t nbytes, 
                                       gasneti_pshm_rank_t target)
{
  gasneti_pshmnet_payload_t *p;
  void *retval = NULL;
  
  gasneti_assert(nbytes <= GASNETI_PSHMNET_MAX_PAYLOAD);

  p = gasneti_pshmnet_alloc(vnet->my_allocator, nbytes);
  if (p != NULL) {
    p->msg = GASNETI_PSHMNET_MSG_NULL;
    p->allocator = vnet->my_allocator;
    retval = &p->data;
  }
  
  return retval;
}


int gasneti_pshmnet_deliver_send_buffer(gasneti_pshmnet_t *vnet, void *buf, 
                                        size_t nbytes, gasneti_pshm_rank_t target)
{
  int retval = -1;
  gasneti_pshmnet_msg_t *q_send_next;
  gasneti_pshmnet_payload_t *p;
  gasneti_pshmnet_queue_t *q = &vnet->out_queues[target];

   /* This code assumes that if the current 'send_node' isn't free yet, there
   * are no free slots in the recipient's queue.  Since there is only one
   * sender, one receiver, and the receiver consumes messages in order, this
   * should be true, so no scan over the list is needed. */
  q_send_next = gasneti_pshmnet_next_msg(q, GASNETI_PSHMNET_EMPTY, 0);
  if (q_send_next) {
    /* fill in message info. Instead of buf we use offset since it
     * will be read by another node wich does not have identical pshmnet
     * memory mapping */
    q_send_next->addr = gasneti_pshm_offset(buf);
    q_send_next->len = nbytes;
    /* set pointer to msg in buffer */
    p = pshmnet_get_struct_addr_from_field_addr(gasneti_pshmnet_payload_t, data, buf);
    gasneti_assert(buf == &p->data);
    p->msg = gasneti_pshm_offset(q_send_next);
    /* Perform write flush before writing ready bit */
    gasneti_pshmnet_state_set(&q_send_next->state, GASNETI_PSHMNET_FULL, GASNETI_ATOMIC_REL);
    retval = 0;
  }

  return retval;
}


int gasneti_pshmnet_recv(gasneti_pshmnet_t *vnet, void **pbuf, size_t *psize, 
                         gasneti_pshm_rank_t *from)
{
  const int nodecount = vnet->nodecount;
  int i;
   
  for (i = 0; i < nodecount; i++) {
    /* Ensure fairness: next check starts with next node: */
#if !defined(GASNETI_THREADS)
    int nodeindex = gasneti_weakatomic_read(&vnet->nextindex, 0);
    int tmp = nodeindex + 1;
    if (tmp == nodecount) tmp = 0;
    gasneti_weakatomic_set(&vnet->nextindex, tmp, 0);
#elif defined(GASNETI_HAVE_ATOMIC_CAS)
    /* 
       There can be a multithread race here.  However, it is harmless
       because the worst that happens is that multiple threads will
       concurrently poll the same node.  Even then the 'nextindex' will
       still advance eventually.
     */
    int nodeindex = gasneti_weakatomic_read(&vnet->nextindex, 0);
    int tmp = nodeindex + 1;
    if (tmp == nodecount) tmp = 0;
    (void)gasneti_weakatomic_compare_and_swap(&vnet->nextindex, nodeindex, tmp, 0);
#elif defined(GASNETI_HAVE_ATOMIC_ADD_SUB)
    /*
       Here we assume that avoiding branches and atomics is worth the cost of
       the '%=' operation, but sacrifice 'continuity' when the counter wraps.
     */
    int nodeindex = gasneti_weakatomic_add(&vnet->nextindex, 1, 0);
    nodeindex %= nodecount;
#else
  #error "Platform is missing both atomic ADD and atomic CAS"
#endif
 
    if (nodeindex != gasneti_pshm_mynode) {
      gasneti_pshmnet_queue_t * const q = &vnet->in_queues[nodeindex];
      gasneti_pshmnet_msg_t *q_recv_next;
      
      q_recv_next = gasneti_pshmnet_next_msg(q, GASNETI_PSHMNET_FULL, GASNETI_ATOMIC_ACQ_IF_TRUE);
      if (q_recv_next) {
        /* Transform the offset in q_recv_next->addr into a real address */
        *pbuf = gasneti_pshm_addr(q_recv_next->addr);
        *psize = q_recv_next->len;

        *from = nodeindex;
        return 0;
      }
    }
  }
  
  return -1;
}


/* Note the current behavior if a user forgets to call this function is
 * NASTY--the message stays marked as state==BUSY, which will cause
 * senders to think the queue is full.  This could cause
 * deadlock and/or lots of confusion (for me it was the latter).
 */
void gasneti_pshmnet_recv_release(gasneti_pshmnet_t *vnet, void *buf)
{
  /* Address we handed out was the addr of the 'data' field */
  gasneti_pshmnet_payload_t *p = 
    pshmnet_get_struct_addr_from_field_addr(gasneti_pshmnet_payload_t,
                                            data, buf);
  gasneti_pshmnet_msg_t *msg;
  gasneti_assert(buf == &p->data);
  gasneti_assert(p && (p->msg != GASNETI_PSHMNET_MSG_NULL) && p->allocator);
  /* mark msg as free */
  msg = gasneti_pshm_addr(p->msg);
#if GASNET_DEBUG
  p->msg = GASNETI_PSHMNET_MSG_NULL;
  gasneti_local_wmb();
#endif
  gasneti_assert(gasneti_pshmnet_state_read(&msg->state) == GASNETI_PSHMNET_BUSY);
  gasneti_pshmnet_state_set(&msg->state, GASNETI_PSHMNET_EMPTY, 0);
  gasneti_pshmnet_free(p->allocator, p);
}


/******************************************************************************
 * PSHMnet bootstrap barrier
 * - TODO: only good a finite number of times before it wraps!
 ******************************************************************************/
void gasneti_pshmnet_bootstrapBarrier(void)
{
  gasneti_atomic_val_t curr, target;

  gasneti_assert(gasneti_pshm_info != NULL);
  gasneti_assert(gasneti_pshm_nodes > 0);

  curr = gasneti_atomic_read(&gasneti_pshm_info->bootstrap_barrier, 0);
  if_pf (curr >= GASNETI_PSHM_BSB_LIMIT) gasnet_exit(1);

  target = gasneti_pshm_nodes + curr - (curr % gasneti_pshm_nodes);
  gasneti_assert_always(target < GASNETI_PSHM_BSB_LIMIT); /* Die if we were ever to reach the limit */

  gasneti_atomic_increment(&gasneti_pshm_info->bootstrap_barrier, 0);

  gasneti_waitwhile((curr = gasneti_atomic_read(&gasneti_pshm_info->bootstrap_barrier, 0)) < target);
  if_pf (curr >= GASNETI_PSHM_BSB_LIMIT) gasnet_exit(1);
}

/******************************************************************************
 * "critical sections" in which we notify peers if we abort() while
 * they are potentially blocked in gasneti_pshmnet_bootstrapBarrier().
 * These DO NOT nest (but there is no checking to ensure that).
 ******************************************************************************/
static gasneti_sighandlerfn_t gasneti_prev_abort_handler = NULL;

static void gasneti_pshm_abort_handler(int sig) {
  gasneti_atomic_set(&gasneti_pshm_info->bootstrap_barrier, GASNETI_PSHM_BSB_LIMIT, 0);

  gasneti_reghandler(SIGABRT, gasneti_prev_abort_handler);
#if HAVE_SIGPROCMASK /* Is this ever NOT the case? */
  { sigset_t new_set, old_set;
    sigemptyset(&new_set);
    sigaddset(&new_set, SIGABRT);
    sigprocmask(SIG_UNBLOCK, &new_set, &old_set);
  }
#endif
  raise(SIGABRT);
}

void gasneti_pshm_cs_enter(void)
{
  gasneti_prev_abort_handler = gasneti_reghandler(SIGABRT, &gasneti_pshm_abort_handler);
}

void gasneti_pshm_cs_leave(void)
{
  gasneti_reghandler(SIGABRT, gasneti_prev_abort_handler);
}

/******************************************************************************
 * Helpers for PSHMnet bootstrap broadcast and exchange
 ******************************************************************************/

/* Sends data to all peers excluding self */
static void gasneti_pshmnet_coll_send(gasneti_pshmnet_t *vnet, void *src, size_t len)
{
  gasneti_pshm_rank_t i;
  void *msg;

  for (i = 0; i < vnet->nodecount; i++) {
    if (i == gasneti_pshm_mynode) continue;
    gasneti_waitwhile (NULL == (msg = gasneti_pshmnet_get_send_buffer(vnet, len, i)));
    memcpy(msg, src, len);
    gasneti_waitwhile (gasneti_pshmnet_deliver_send_buffer(vnet, msg, len, i));
  }
}

/* Recive data from any peer excluding self, placing data according to srcidx*stride */
static void gasneti_pshmnet_coll_recv(gasneti_pshmnet_t *vnet, size_t stride, void *dest)
{
  gasneti_pshm_rank_t from;
  void *msg, *dest_elem;
  size_t len;

  gasneti_waitwhile (gasneti_pshmnet_recv(vnet, &msg, &len, &from));
  dest_elem = (void*)((uintptr_t)dest + (stride * from));
  memcpy(dest_elem, msg, len);
  gasneti_pshmnet_recv_release(vnet, msg);
}

/******************************************************************************
 * PSHMnet bootstrap broadcast
 * - Rootpshmnode is supernode-local rank
 * - Barriers ensure ordering w.r.t sends that precede or follow
 ******************************************************************************/
void gasneti_pshmnet_bootstrapBroadcast(gasneti_pshmnet_t *vnet, void *src, 
                                        size_t len, void *dst, int rootpshmnode)
{
  uintptr_t src_addr = (uintptr_t)src;
  uintptr_t dst_addr = (uintptr_t)dst;
  size_t remain = len;

  gasneti_assert(vnet != NULL);
  gasneti_assert(vnet->nodecount == gasneti_pshm_nodes);

  while (remain) {
    size_t nbytes = MIN(remain, GASNETI_PSHMNET_MAX_PAYLOAD);

    gasneti_pshmnet_bootstrapBarrier();
    if (gasneti_pshm_mynode == rootpshmnode) {
      gasneti_pshmnet_coll_send(vnet, (void*)src_addr, nbytes);
    } else {
      gasneti_pshmnet_coll_recv(vnet, 0, (void*)dst_addr);
    }

    src_addr += nbytes;
    dst_addr += nbytes;
    remain -= nbytes;
  }
  memmove(dst, src, len);
}

/******************************************************************************
 * PSHMnet bootstrap exchange
 * - Barriers ensure ordering w.r.t sends that precede or follow
 ******************************************************************************/
void gasneti_pshmnet_bootstrapExchange(gasneti_pshmnet_t *vnet, void *src, 
                                       size_t len, void *dst)
{
  uintptr_t src_addr = (uintptr_t)src;
  uintptr_t dst_addr = (uintptr_t)dst;
  size_t remain = len;

  gasneti_assert(vnet != NULL);
  gasneti_assert(vnet->nodecount == gasneti_pshm_nodes);

  /* All nodes broadcast their contribution in turn */
  while (remain) {
    size_t nbytes = MIN(remain, GASNETI_PSHMNET_MAX_PAYLOAD);
    gasneti_pshm_rank_t i;

    gasneti_pshmnet_bootstrapBarrier(); 
    for (i = 0; i < vnet->nodecount; i++) {
      if (gasneti_pshm_mynode == i) {
        gasneti_pshmnet_coll_send(vnet, (void*)src_addr, nbytes);
      } else {
        gasneti_pshmnet_coll_recv(vnet, len, (void*)dst_addr);
      }
    }

    src_addr += nbytes;
    dst_addr += nbytes;
    remain -= nbytes;
  }
  memmove((void*)((uintptr_t)dst + (gasneti_pshm_mynode*len)), src, len);
}


/******************************************************************************
 * Allocator implementation
 ******************************************************************************/

static gasneti_pshmnet_allocator_t *gasneti_pshmnet_init_allocator(void *region, size_t len)
{
  int count = len >> GASNETI_PSHMNET_PAGESHIFT;
  gasneti_pshmnet_allocator_block_t *tmp;

  /* This implementation doesn't need to put allocator within shared memory.
   * If a later one does, consider increasing the size returned by
   * get_queue_mem()
   */
  gasneti_pshmnet_allocator_t *a = gasneti_malloc(sizeof(gasneti_pshmnet_allocator_t) +
                                                  (count-1) * sizeof(unsigned int));
  gasneti_leak(a);

  /* make sure we've arranged for page alignment */
  gasneti_assert_align(GASNETI_PSHMNET_ALLOC_MAXSZ, GASNETI_PSHMNET_PAGESIZE);
  gasneti_assert_align(region, GASNETI_PSHMNET_PAGESIZE);

  /* Initial state is one large free block */
  a->next = 0;
  a->count = count;
  a->length[0] = count;
  a->region = tmp = region;
  gasneti_atomic_set(&tmp->in_use, 0, 0);

  return a;
}


/* Basic page-granular first-fit allocator
   This allocator is NOT thread-safe - the callers are responsible for serialization. */
static void * gasneti_pshmnet_alloc(gasneti_pshmnet_allocator_t *a, size_t nbytes)
{
  void *retval = NULL;
  unsigned int curr;
  unsigned int needed;
  int remain;
  void *region = a->region;

  nbytes += offsetof(gasneti_pshmnet_allocator_block_t, payload.data);
  gasneti_assert(nbytes <= GASNETI_PSHMNET_ALLOC_MAXSZ);

  needed = (nbytes + GASNETI_PSHMNET_PAGESIZE - 1) >> GASNETI_PSHMNET_PAGESHIFT;
  gasneti_assert(needed <= GASNETI_PSHMNET_ALLOC_MAXPG);

  curr = a->next;
  remain = a->count;
  do {
    unsigned int length = a->length[curr];
    gasneti_pshmnet_allocator_block_t *next_block;
    gasneti_pshmnet_allocator_block_t * const block = 
            (gasneti_pshmnet_allocator_block_t*)
                      ((uintptr_t)region + (curr << GASNETI_PSHMNET_PAGESHIFT));

    if (!gasneti_atomic_read(&block->in_use, GASNETI_ATOMIC_ACQ)) {
      while (length < needed) {
        unsigned int next = curr + length;
        gasneti_assert (next <= a->count);
        if (next == a->count) break; /* hit end of region */
        next_block = (gasneti_pshmnet_allocator_block_t*)
                     ((uintptr_t)block + (length << GASNETI_PSHMNET_PAGESHIFT));
        if (gasneti_atomic_read(&next_block->in_use, GASNETI_ATOMIC_ACQ)) break; /* hit busy block */
        length += a->length[next];
      }

      if (length >= needed) {
        unsigned int next = curr + needed;

        if (length > needed) { /* Split it */
          gasneti_assert (next < a->count);
          a->length[next] = length - needed;
          next_block = (gasneti_pshmnet_allocator_block_t*)
                       ((uintptr_t)block + (needed << GASNETI_PSHMNET_PAGESHIFT));
          gasneti_atomic_set(&next_block->in_use, 0, 0);
        }

        a->length[curr] = needed;
        gasneti_atomic_set(&block->in_use, 1, 0);
        retval = &block->payload;

        if ((curr =next) == a->count) curr = 0;
        break; /* do {} while (remain); */
      }

      /* Assume write is cheaper than brancing again when length is unchanged */
      a->length[curr] = length;
    }

    remain -= length;
    if ((curr += length) == a->count) curr = 0;
  } while (remain > 0); /* could be negative if merging took us past our starting point */
  a->next = curr;
  return retval;
}

static void gasneti_pshmnet_free(gasneti_pshmnet_allocator_t *a, void *p)
{
  /* We don't need the allocator ptr, but other implementations might  */

  /* Address we handed out was the addr of the 'payload_t' field */
  gasneti_pshmnet_allocator_block_t *block = 
      pshmnet_get_struct_addr_from_field_addr(gasneti_pshmnet_allocator_block_t,
                                              payload, p);
  gasneti_assert(p == &block->payload);
  /* assert block is page-aligned */
  gasneti_assert( (((uintptr_t)block) % GASNETI_PSHMNET_PAGESIZE) == 0);

  gasneti_atomic_set(&block->in_use, 0, GASNETI_ATOMIC_REL);
}

/******************************************************************************
 * AMPSHM:  Active Message API over PSHMnet
 ******************************************************************************/

/* The mediumdata field may not be aligned */
#define GASNETI_AMPSHM_MSG_MEDDATA_OFFSET \
   (offsetof(gasneti_pshmnet_allocator_block_t, payload.data.Medium.mediumdata)&7)
#define GASNETI_AMPSHM_MSG_MEDDATA_SHIFT \
   (GASNETI_AMPSHM_MSG_MEDDATA_OFFSET?(8-GASNETI_AMPSHM_MSG_MEDDATA_OFFSET):0)

#define GASNETI_AMPSHM_MSG_CATEGORY(msg)      (((gasneti_AMPSHM_msg_t*)msg)->category)
#define GASNETI_AMPSHM_MSG_HANDLERID(msg)     (((gasneti_AMPSHM_msg_t*)msg)->handler_id)
#define GASNETI_AMPSHM_MSG_NUMARGS(msg)       (((gasneti_AMPSHM_msg_t*)msg)->numargs)
#define GASNETI_AMPSHM_MSG_SOURCE(msg)        (((gasneti_AMPSHM_msg_t*)msg)->source)
#define GASNETI_AMPSHM_MSG_ARGS(msg)          (((gasneti_AMPSHM_msg_t*)msg)->args)
#define GASNETI_AMPSHM_MSG_MED_NUMBYTES(msg)  (((gasneti_AMPSHM_medmsg_t*)msg)->numbytes)
#define GASNETI_AMPSHM_MSG_MED_DATA(msg)      ((((gasneti_AMPSHM_medmsg_t*)msg)->mediumdata) + \
                                               GASNETI_AMPSHM_MSG_MEDDATA_SHIFT)
#define GASNETI_AMPSHM_MSG_LONG_NUMBYTES(msg) (((gasneti_AMPSHM_longmsg_t*)msg)->numbytes)
#define GASNETI_AMPSHM_MSG_LONG_DATA(msg)     (((gasneti_AMPSHM_longmsg_t*)msg)->longdata)

#define GASNETI_AMPSHM_MAX_REPLY_PER_POLL 10
#define GASNETI_AMPSHM_MAX_REQUEST_PER_POLL 10

#ifndef GASNETC_ENTERING_HANDLER_HOOK
  /* extern void enterHook(int cat, int isReq, int handlerId, gasnet_token_t *token,
   *                       void *buf, size_t nbytes, int numargs, gasnet_handlerarg_t *args);
   */
  #define GASNETC_ENTERING_HANDLER_HOOK(cat,isReq,handlerId,token,buf,nbytes,numargs,args) ((void)0)
#endif
#ifndef GASNETC_LEAVING_HANDLER_HOOK
  /* extern void leaveHook(int cat, int isReq);
   */
  #define GASNETC_LEAVING_HANDLER_HOOK(cat,isReq) ((void)0)
#endif

/* ------------------------------------------------------------------------------------ */
GASNETI_INLINE(gasneti_AMPSHM_service_incoming_msg)
int gasneti_AMPSHM_service_incoming_msg(gasneti_pshmnet_t *vnet, int isReq)
{
  void *msg;
  size_t msgsz;
  gasneti_pshm_rank_t from;
  int category;
  gasnetc_handler_t handler_id;
  gasneti_handler_fn_t handler_fn;
  int numargs;
  gasnet_handlerarg_t *args;
  gasnet_token_t token;

  gasneti_assert(vnet != NULL);

  if (gasneti_pshmnet_recv(vnet, &msg, &msgsz, &from))
    return -1;
  GASNETI_PSHMNET_FE_DEC(vnet,gasneti_pshm_mynode);

  token = gasnetc_token_create(GASNETI_AMPSHM_MSG_SOURCE(msg), isReq);
  category = GASNETI_AMPSHM_MSG_CATEGORY(msg);
  gasneti_assert((category == gasnetc_Short) || 
                 (category == gasnetc_Medium) || 
                 (category == gasnetc_Long));
  handler_id = GASNETI_AMPSHM_MSG_HANDLERID(msg);
  handler_fn = gasnetc_get_handler(handler_id);
  numargs = GASNETI_AMPSHM_MSG_NUMARGS(msg);
  args = GASNETI_AMPSHM_MSG_ARGS(msg);

  switch (category) {
    case gasnetc_Short:
      { 
        GASNETC_ENTERING_HANDLER_HOOK(category,isReq,handler_id,token,NULL,0,numargs,args);
        GASNETI_RUN_HANDLER_SHORT(isReq,handler_id,handler_fn,token,args,numargs);
      }
      break;
    case gasnetc_Medium:
      {
        void * data = GASNETI_AMPSHM_MSG_MED_DATA(msg);
        size_t nbytes = GASNETI_AMPSHM_MSG_MED_NUMBYTES(msg);
        GASNETC_ENTERING_HANDLER_HOOK(category,isReq,handler_id,token,data,nbytes,numargs,args);
        GASNETI_RUN_HANDLER_MEDIUM(
          isReq,handler_id,handler_fn,token,args,numargs,data,nbytes);
      }
      break;
    case gasnetc_Long:
      { 
        void * data = GASNETI_AMPSHM_MSG_LONG_DATA(msg);
        size_t nbytes = GASNETI_AMPSHM_MSG_LONG_NUMBYTES(msg);
        GASNETC_ENTERING_HANDLER_HOOK(category,isReq,handler_id,token,data,nbytes,numargs,args);
        GASNETI_RUN_HANDLER_LONG(
            isReq,handler_id,handler_fn,token,args,numargs,data,nbytes);
      }
      break;
  }
  GASNETC_LEAVING_HANDLER_HOOK(category,isReq);
  gasnetc_token_destroy(token);
  gasneti_pshmnet_recv_release(vnet, msg);
  return 0;
}

/* ------------------------------------------------------------------------------------ */
int gasneti_AMPSHMPoll(int repliesOnly)
{
  int i = 0;

#if 0
  /* We skip CHECKATTACH to allow "early" internal use by conduits. */
  GASNETI_CHECKATTACH();
#endif

  if (GASNETI_PSHMNET_FE_READ(gasneti_reply_pshmnet,gasneti_pshm_mynode)) {
    GASNETI_PSHMNET_FE_ACK(gasneti_reply_pshmnet,gasneti_pshm_mynode);
    for (i = 0; i < GASNETI_AMPSHM_MAX_REPLY_PER_POLL; i++) 
      if (gasneti_AMPSHM_service_incoming_msg(gasneti_reply_pshmnet, 0))
        break;
    GASNETI_PSHMNET_FE_SUB(gasneti_reply_pshmnet,gasneti_pshm_mynode,i);
  }
  if (!repliesOnly && GASNETI_PSHMNET_FE_READ(gasneti_request_pshmnet,gasneti_pshm_mynode)) {
    GASNETI_PSHMNET_FE_ACK(gasneti_request_pshmnet,gasneti_pshm_mynode);
    for (i = 0; i < GASNETI_AMPSHM_MAX_REQUEST_PER_POLL; i++) 
      if (gasneti_AMPSHM_service_incoming_msg(gasneti_request_pshmnet, 1))
        break;
    GASNETI_PSHMNET_FE_SUB(gasneti_request_pshmnet,gasneti_pshm_mynode,i);
  }
  return GASNET_OK;
}

/* ------------------------------------------------------------------------------------ */
/*
 * Active Message Request Functions
 * ================================
 */

/* Loopback AMs use buffers from this free pool.
 * Worst case this pool grows to two per threads (one request and one reply).
 * TODO: per-thread buffers (as in smp-conduit) would remove contention, but
 * requires mofiying the conduit-specific code for threaddata.
 */
static gasneti_lifo_head_t loopback_freepool = GASNETI_LIFO_INITIALIZER;

int gasnetc_AMPSHM_ReqRepGeneric(int category, int isReq, gasnet_node_t dest,
                                 gasnetc_handler_t handler, void *source_addr, size_t nbytes, 
                                 void *dest_addr, int numargs, va_list argptr) 
{
  gasneti_pshmnet_t *vnet = (isReq ? gasneti_request_pshmnet : gasneti_reply_pshmnet);
  gasneti_pshm_rank_t target = gasneti_pshm_local_rank(dest);
  size_t msgsz = 0;
  int i;
  void *msg;
  int loopback = (dest == gasneti_mynode);

  gasneti_assert(vnet != NULL);
  gasneti_assert(target < gasneti_pshm_nodes);

  if (loopback) {
    msg = gasneti_lifo_pop(&loopback_freepool);
    if_pf (msg == NULL) {
      /* Grow the free pool with buffers sized and aligned for the largest Medium */
      void *tmp = gasneti_malloc(sizeof(gasneti_AMPSHM_medmsg_t)+7);
      uintptr_t offset = (uintptr_t)GASNETI_AMPSHM_MSG_MED_DATA(tmp) & 7;
      /* Align the (macro-adjusted) Medium payload field, not the msg itself */
      msg = (void*)((uintptr_t)tmp + (offset ? (8-offset) : 0));
    }
  } else {
    static gasneti_mutex_t req_lock = GASNETI_MUTEX_INITIALIZER;
    static gasneti_mutex_t rep_lock = GASNETI_MUTEX_INITIALIZER;
    gasneti_mutex_t *lock;

    /* calculate size of buffer needed */
    switch (category) {
      case gasnetc_Short:
        msgsz = sizeof(gasneti_AMPSHM_shortmsg_t);
        break;
      case gasnetc_Medium:
        msgsz = sizeof(gasneti_AMPSHM_medmsg_t) - (GASNETC_MAX_MEDIUM_PSHM - nbytes);
        break;
      case gasnetc_Long:
        msgsz = sizeof(gasneti_AMPSHM_longmsg_t);
        break;
      default:
        gasneti_fatalerror("internal error: unknown msg category");
    }
    gasneti_assert(msgsz <= sizeof(gasneti_AMPSHM_maxmsg_t)); 

    /* Get buffer, poll if busy.
       Lock serializes allocation so small messages can't starve large ones */
    lock = isReq ? &req_lock : &rep_lock;
    gasneti_mutex_lock(lock);
    while (!(msg = gasneti_pshmnet_get_send_buffer(vnet, msgsz, target))) {
      /* If reply, only poll reply network: avoids deadlock  */
      if (isReq) gasnetc_AMPoll(); /* No progress functions */
      else gasneti_AMPSHMPoll(1);
      GASNETI_WAITHOOK();
    }
    gasneti_mutex_unlock(lock);
  }

  /* Fill in message */
  GASNETI_AMPSHM_MSG_CATEGORY(msg) = category;
  GASNETI_AMPSHM_MSG_HANDLERID(msg) = handler;
  GASNETI_AMPSHM_MSG_NUMARGS(msg) = numargs;
  GASNETI_AMPSHM_MSG_SOURCE(msg) = gasneti_mynode;
  for(i = 0; i < numargs; i++) 
    GASNETI_AMPSHM_MSG_ARGS(msg)[i] = (gasnet_handlerarg_t)va_arg(argptr, int);

  /* Detect truncation if our field widths were too small */
  gasneti_assert( GASNETI_AMPSHM_MSG_CATEGORY(msg) == category );
  gasneti_assert( GASNETI_AMPSHM_MSG_NUMARGS(msg) == numargs );

  switch (category) {
    case gasnetc_Short:
      break;
    case gasnetc_Medium:
      GASNETI_AMPSHM_MSG_MED_NUMBYTES(msg) = nbytes;
      gasneti_assert( GASNETI_AMPSHM_MSG_MED_NUMBYTES(msg) == nbytes ); /* truncation check */
      memcpy(GASNETI_AMPSHM_MSG_MED_DATA(msg), source_addr, nbytes);
      break;
    case gasnetc_Long: {
      void *local_dest_addr = gasneti_pshm_addr2local(dest, dest_addr);

      GASNETI_AMPSHM_MSG_LONG_DATA(msg) = dest_addr; 
      GASNETI_AMPSHM_MSG_LONG_NUMBYTES(msg) = nbytes;
      gasneti_assert( GASNETI_AMPSHM_MSG_LONG_NUMBYTES(msg) == nbytes ); /* truncation check */
      /* deliver_msg call, below, contains write flush, so don't need here */
      memcpy(local_dest_addr, source_addr, nbytes);
      break;
    }
  }

  /* Deliver message */
  if (loopback) {
    gasneti_handler_fn_t handler_fn = gasnetc_get_handler(handler); 
    gasnet_token_t token = gasnetc_token_create(gasneti_mynode, isReq);
    gasnet_handlerarg_t *args = GASNETI_AMPSHM_MSG_ARGS(msg);
    switch (category) {
      case gasnetc_Short:
        GASNETC_ENTERING_HANDLER_HOOK(category,isReq,handler,token,NULL,0,numargs,args);
        GASNETI_RUN_HANDLER_SHORT(isReq,handler,handler_fn,token,args,numargs);

        break;
      case gasnetc_Medium:
        GASNETC_ENTERING_HANDLER_HOOK(category,isReq,handler,token,
                                      GASNETI_AMPSHM_MSG_MED_DATA(msg),nbytes,numargs,args);
        GASNETI_RUN_HANDLER_MEDIUM(isReq, handler, handler_fn, token, args, numargs,
                                   GASNETI_AMPSHM_MSG_MED_DATA(msg), nbytes);
        break;
      case gasnetc_Long:
        gasneti_local_wmb(); /* sync memcpy, above */
        GASNETC_ENTERING_HANDLER_HOOK(category,isReq,handler,token,dest_addr,nbytes,numargs,args);
        GASNETI_RUN_HANDLER_LONG(isReq, handler, handler_fn, token, args, numargs,
                                 dest_addr, nbytes);
        break;
    }
    GASNETC_LEAVING_HANDLER_HOOK(category,isReq);
    gasneti_lifo_push(&loopback_freepool, msg);
    gasnetc_token_destroy(token);
  } else {
    
    while (gasneti_pshmnet_deliver_send_buffer(vnet, msg, msgsz, target)) {
      /* If reply, only poll reply network: avoids deadlock  */
      if (isReq) gasnetc_AMPoll(); /* No progress functions */
      else gasneti_AMPSHMPoll(1);
      GASNETI_WAITHOOK();
    }
    GASNETI_PSHMNET_FE_INC(vnet,target);
  }
  return GASNET_OK;
}

#endif /* GASNET_PSHM */
