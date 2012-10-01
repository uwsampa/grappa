// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "uts.h"

#include <SoftXMT.hpp>
#include <Cache.hpp>
#include <ParallelLoop.hpp>
#include <ForkJoin.hpp>
#include <GlobalAllocator.hpp>
#include <Addressing.hpp>
#include <PerformanceTools.hpp>
#include <GlobalTaskJoiner.hpp>
#include <AsyncParallelFor.hpp>

#include <iostream>

#include <stdlib.h> // for memset

/* *******************************************************************************************
 * Unbalanced Tree Search in-memory (UTS-mem). This is an extension of the UTS benchmark, where
 * we do the implicit traversal of UTS to build a tree in memory. The timed portion
 * is to traverse this tree. In this way, UTS-mem tests both dynamic load balancing and 
 * random access.

 * This is a Grappa implementation of UTS-mem.
***********************************************************************************************/


// UTS-mem Grappa implementation specific command line parameters (using gflags instead of UTS impl_params)
DEFINE_int64( vertices_size, 1<<20, "Upper bound count of vertices" );
DEFINE_bool( verify_tree, true, "Verify the generated tree" );

// Parallel granularities for important parallel for-loops
#define VERIFY_THRESHOLD ((int64_t) 4)
#define CREATE_THRESHOLD ((int64_t) 1)
// threshold for tree search is specified by Grappa option --async_par_for_threshold

// declare Grappa stealing parameters
DECLARE_bool( steal );
DECLARE_int32( chunk_size );



/* ****************************************************************************
 *  UTS interface definitions
 * ****************************************************************************/
char * impl_getName() { return "Grappa"; }
int    impl_paramsToStr(char * strBuf, int ind) {
  ind += sprintf(strBuf+ind, "Execution strategy:  ");

  ind += sprintf(strBuf+ind, "Parallel search using %d processes\n", SoftXMT_nodes());
  ind += sprintf(strBuf+ind, "   up to %d threads per core\n", FLAGS_num_starting_workers );

  if (FLAGS_steal) {
    ind += sprintf(strBuf+ind, "    Dynamic load balance by work stealing, chunk size = %d nodes\n", FLAGS_chunk_size);
  } else {
    ind += sprintf(strBuf+ind, "   No dynamic load balancing.\n");
  }
  /* 
     if (distributeInitial) {
     ind += sprintf(strBuf+ind, "   Distribute first generation.\n");
     } else {
     ind += sprintf(strBuf+ind, "   No initial distribution.\n");
     }
     */

  return ind;
}

void   impl_helpMessage() {
}

int impl_parseParam(char * param, char * value) { 
  // not using this mechanism for implementation params (see gflags defines above)
  return 1;
}

void impl_abort(int err) {
  exit(err);
  //SoftXMT_finish( err );//TODO KILL ALL NODES
} 


/* ****************************************************************************
 * Global variables that every Node needs a local copy of
 *****************************************************************************/
// command line args to initialize UTS on every Node
int global_argc;
char **global_argv;

// Global counters for the tree generation
uint64_t global_id; // only Node 0 matters
uint64_t global_child_index = 0; // only Node 0 matters

// every Node keeps the address of these Node 0 global counters
GlobalAddress<uint64_t> global_id_ga;
GlobalAddress<uint64_t> global_child_index_ga;

// Node-local counters for asynch-for generate and search
uint64_t tj_num_gen_nodes = 0;
uint64_t tj_num_searched_nodes = 0;

// GlobalTaskJoiner implementation option
// setting this to false uses fine-grain joins parallel loop and not well tested
const bool opt_ff = true;

// these are for doing a join on all
// vertices being visited in the noJoin search
uint64_t node_private_num_vertices = 0;
Semaphore * node_private_vertex_sem;

/// verification counts
uint64_t local_verify_children_count = 0;


/* ****************************************************************************
 * Tree
 ******************************************************************************/
struct vertex_t {
  int64_t numChildren;
  int64_t childIndex;
};
std::ostream& operator<< ( std::ostream& out, const vertex_t& v ) {
  out << "vertex{numChildren=" << v.numChildren
    << ", childIndex=" << v.childIndex
    << "} ";
  return out;
}
std::ostream& operator<< ( std::ostream& out, const uts::Node& n ) {
  char debug_str[1000];
  out << "uts::Node{type=" << n.type
    << ", height=" << n.height
    << ", numChildren=" << n.numChildren
    << ", id=" << n.id
    << ", state=" << rng_showstate((RNG_state*)n.state.state, debug_str)
    << "} ";
  return out;
}

// Shared tree structures
GlobalAddress<vertex_t> Vertex; // vertices
GlobalAddress<int64_t> Child; // child ids (edgelists)
GlobalAddress<uts::Node> Tree_Nodes; // temporary for building the tree
GlobalAddress<int64_t> Payload;

/// Store results of tree traversals
struct Result {
  int64_t maxdepth, size, leaves; 
};

/* ****************************************************************************
 * Initialization functions
 * ***************************************************************************/

// Initialization on each node
  LOOP_FUNCTOR( init_node_f, nid, ((GlobalAddress<vertex_t>, Vertex_addr))
      ((GlobalAddress<int64_t>, Child_addr))
      ((GlobalAddress<uts::Node>, Tree_Nodes_addr)) ) {
    global_id_ga = make_global( &global_id, 0 );
    global_child_index_ga = make_global( &global_child_index, 0 );

    Vertex = Vertex_addr;
    Child = Child_addr;
    Tree_Nodes = Tree_Nodes_addr;

    // initialize UTS with args
    LOG(INFO) << "Initializing UTS";
    uts_parseParams(global_argc, global_argv);

    // initialize counters with profiler
    SoftXMT_add_profiling_counter( &tj_num_gen_nodes, "uts_num_gen_nodes", "utsgennodes", false, 0 );
    SoftXMT_add_profiling_counter( &tj_num_searched_nodes, "uts_num_searched_nodes", "utssearchnodes", false, 0 );
  }

// initialize payload info on each node
LOOP_FUNCTOR( payinit_f, nid, ((GlobalAddress<int64_t>, Payload_addr)) ) {
  Payload = Payload_addr;
}



/* ************************************************************
 * GlobalTaskJoiner feed forward versions of create and search
 *
 * ***********************************************************/


void tj_create_children( uts::Node * parent );

// loop body (ranged for caching) of for loop that creates all the new children
void tj_create_vertex( int64_t start, int64_t num, GlobalAddress<void*> pp ) {

  // acquiring these two parent items is the only thing shared by doing
  // num > 1 iterations

  int64_t unpacked_parent_id = reinterpret_cast<int64_t>(pp.pointer());
  uts::Node parent_storage;
  Incoherent<uts::Node>::RO parentc( Tree_Nodes + unpacked_parent_id, 1, &parent_storage);

  // TODO: this is a lot of extra reads just to obtain childid0; easier to have args but feed forward..
  int64_t p_childIndex = SoftXMT_delegate_read_word( global_pointer_to_member( Vertex+unpacked_parent_id, &vertex_t::childIndex) );
  int64_t p_childid0 = SoftXMT_delegate_read_word( Child + p_childIndex );

  parentc.block_until_acquired();
  int64_t childType = uts_childType(&parent_storage);// need to pass in normal pointer
  int64_t height = (*parentc).height + 1;

  // now perform the actual loop body iterations
  for ( int64_t cn=0; cn<num; cn++ ) {
    // new node v
    
    int64_t childnum = start+cn;
    int64_t v_id = p_childid0 + childnum;
    DVLOG(5) << "v_id = p_child0(" << p_childid0 << ") + childnum(" << childnum << ")";
    uts::Node v_storage;

    // set v's properties
    v_storage.type = childType;
    v_storage.height = height;
    v_storage.id = v_id;

    // initialize the RNG state of v
    for (int k=0; k<computeGranularity; k++) {
      // un-const cast
      rng_spawn((RNG_state*)(*parentc).state.state, v_storage.state.state, childnum); 
    }

    // create the children of v
    tj_create_children( &v_storage );
  }
}

/// Create a tree below the given parent vertex
///
/// This is a feed forward implementation of creation
void tj_create_children( uts::Node * parent ) {
  tj_num_gen_nodes++;

  DVLOG(5) << "creating children for id=" << parent->id;
  Incoherent<uts::Node>::WO v( Tree_Nodes + parent->id, 1, parent );

  int64_t numChildren = uts_numChildren( parent );
  (*v).numChildren = numChildren;
  v.start_release(); // done writing to v locally


  /* Assign fresh unique ids for the children: */
  /* TODO: make global_id_ga not a hotspot */
  int64_t childid0 = SoftXMT_delegate_fetch_and_add_word( global_id_ga, numChildren );
  VLOG_EVERY_N(2, 250000) << "new childids: [" << childid0 
    << ", " << (childid0 + numChildren - 1) 
    << "]";

  /* Record ids and indices of the children: */ 
  /* TODO: make global_child_index_ga not a hotspot */
  int64_t index = SoftXMT_delegate_fetch_and_add_word( global_child_index_ga, numChildren );

  /* store parts of parent that will be read in tree search */
  vertex_t vvert_storage;
  Incoherent<vertex_t>::WO vvert( Vertex + parent->id, 1, &vvert_storage );
  (*vvert).numChildren = numChildren;
  (*vvert).childIndex = index;
  vvert.start_release();  // done writing to vvert locally

  /* write children indices */
  if ( numChildren > 0 ) { 
    // Write out as one block
    // Incoherent is fine because this section of Child is reserved
    int64_t childIds_storage[numChildren];
    Incoherent<int64_t>::WO childIds( Child + index, numChildren, childIds_storage );

    for (int i = 0; i < numChildren; i++) {
      DVLOG(5) << "[in progress] writing out childIds [i=" 
        << i << "]="<< childid0 + i 
        << "\n\t\tfor parent=" << *parent;
      childIds[i] = childid0 + i;
    }
    DVLOG(5) << "[done] writing out childIds [" << childid0 
      << "," << childid0 + (numChildren-1) 
      << "]\n\t\tfor parent=" << *parent;

    // TODO: I actually just want the WO childIds to start_release() at the
    // end of this scope, not block
  } else {
    DVLOG(5) << "[done] no childIds to write\n\t\tfor parent=" << *parent;
  }

  // make sure v and vvert.childIndex has been written out before recursing
  v.block_until_released();
  vvert.block_until_released();
  DVLOG(5) << "[done] released vertex " << vvert_storage << " id=" << parent->id;

  GlobalAddress<void*> packedParentId = make_global( reinterpret_cast<void**>( parent->id ) );
  async_parallel_for<void*, tj_create_vertex, joinerSpawn_hack<void*,tj_create_vertex,CREATE_THRESHOLD>,CREATE_THRESHOLD >((int64_t)0, numChildren, packedParentId);
}

/// Function to call on all Nodes to start tj_create_children
LOOP_FUNCTION(create_func,nid) {
  global_joiner.reset();
  if (nid==0) {
    global_id = 0;
    uts::Node root;
    uts_initRoot( &root, type );
    root.id = global_id++;
    tj_create_children( &root );
  }
  global_joiner.wait();
}

void search_children( int64_t start, int64_t iters );

/// Traverse the tree starting from Vertex[start]
///
/// This is the fastest, best tested Grappa implementation
/// of the in-memory traversal
void search_vertex( int64_t start, int64_t iters ) {
  for ( int64_t id = start; id<start+iters; id++ ) {
    tj_num_searched_nodes++;
    int64_t numChildren;
    int64_t childIndex;

    GlobalAddress<vertex_t> v_addr = Vertex + id;
    { 
      vertex_t v_storage;
      Incoherent<vertex_t>::RO v( v_addr, 1, &v_storage );
      /* (v) = Vertex[id] */

      numChildren = (*v).numChildren;
      childIndex = (*v).childIndex;
    }

    // process children
    async_parallel_for<search_children, joinerSpawn<search_children,ASYNC_PAR_FOR_DEFAULT>,ASYNC_PAR_FOR_DEFAULT >( childIndex, numChildren );
  }
}

// wrapper to count finished tasks since using SoftXMT_publicTask() in search_children instead of an async_parallel_for with joinerSpawn
void wrapped_search_vertex( int64_t start, int64_t iters, GlobalAddress<GlobalTaskJoiner> joiner ) {
  search_vertex( start, iters );
  global_joiner.remoteSignal( joiner );
}

// loop body (ranged for caching) for the parallel for loop over children list
void search_children( int64_t start, int64_t iters ) {
  // all iterations in the range share the relevant chunk of the child list
  int64_t childid_storage[iters];
  Incoherent<int64_t>::RO childids( Child + start, iters, childid_storage );

  // spawning tasks serially since iters < paralell threshold (i.e. constant)
  for ( int64_t i=0; i<iters; i++ ) {
    global_joiner.registerTask();
    SoftXMT_publicTask( &wrapped_search_vertex, childids[i], (int64_t)1, make_global( &global_joiner) );
  }
}

/// Function to call on all Nodes to start search_vertex
LOOP_FUNCTOR(search_func, nid, ((int64_t, root)) ) {
  global_joiner.reset();

  // node0 starts search by visiting root
  if ( nid==0 ) {
    search_vertex( root, 1 );
  }

  global_joiner.wait();
}


///////////////////////////////////////////////////////////////
// Tree generation verification
///////////////////////////////////////////////////////////////

void verifyChild(int64_t start, int64_t num) {
  int64_t c_stor[num];
  Incoherent<int64_t>::RO c( Child + start, num, c_stor );
  for (int64_t i=0; i<num; i++) {
    CHECK( c[i] > 0 ) << "Child[" << i << "] = " << c[i]; // > 0 because root is never child
  }
}

void verifyVertex(int64_t start, int64_t num) {
  vertex_t v_stor[num];
  Incoherent<vertex_t>::RO v( Vertex + start, num, v_stor );
  for (int64_t i=0; i<num; i++) {
    CHECK( v[i].numChildren >= 0 ) << "Vertex[" << i << "].numChildren = " << v[i].numChildren; // >= 0 because must be initialized
    CHECK( v[i].childIndex >= 0 ) << "Vertex[" << i << "].childIndex = " << v[i].childIndex; // >= 0 because must be initialized

    local_verify_children_count += v[i].numChildren;
  }
}

LOOP_FUNCTOR(verify_f,nid, ((uint64_t, num_vertices_gen)) ) {
  // verify Child[]
  global_joiner.reset();
  if (nid==0) {
    async_parallel_for<verifyChild, joinerSpawn<verifyChild, VERIFY_THRESHOLD>, VERIFY_THRESHOLD >( 0, num_vertices_gen-1 ); // -1, root is not in it
  }
  global_joiner.wait();

  // verify Vertex[]
  global_joiner.reset();
  if (nid==0) {
    async_parallel_for<verifyVertex, joinerSpawn<verifyVertex, VERIFY_THRESHOLD>, VERIFY_THRESHOLD >( 0, num_vertices_gen ); // no -1, include root
  }
  global_joiner.wait();
}

void verify_generation( uint64_t num_vert ) {
  verify_f verf;
  verf.num_vertices_gen = num_vert;
  fork_join_custom(&verf);

  uint64_t total_numChildren = 0;
  // count numChildren entries
  for (Node n=0; n<SoftXMT_nodes(); n++) {
    uint64_t nc = SoftXMT_delegate_read_word( make_global( &local_verify_children_count, n ) );
    //VLOG(5) << "Node " << n << " counted " << nc << " children";
    total_numChildren += nc;
  }

  CHECK( total_numChildren == num_vert-1 ) << "verify got " << total_numChildren <<", expected " << num_vert-1;
}

void safeinitChild( int64_t start, int64_t num ) {
  int64_t c_stor[num];
  VLOG(5) << "initializing Child[ " << start << " , " << start+(num-1) << " ]";
  Incoherent<int64_t>::WO c( Child + start, num, c_stor );
  for (int64_t i=0; i<num; i++) {
    c[i] = -1; 
  }
}

void safeinitVertex( int64_t start, int64_t num ) {
  vertex_t v_stor[num];
  VLOG(5) << "initializing Vertex[ " << start << " , " << start+(num-1) << " ]";
  Incoherent<vertex_t>::WO v( Vertex + start, num, v_stor );
  for (int64_t i=0; i<num; i++) {
    v[i].numChildren = -2;
    v[i].childIndex = -3;
  }
}

LOOP_FUNCTION(safe_init_f,nid) {
  // init Child[]
  global_joiner.reset();
  if (nid==0) {
    async_parallel_for<safeinitChild, joinerSpawn<safeinitChild, VERIFY_THRESHOLD>, VERIFY_THRESHOLD >( 0, FLAGS_vertices_size ); // entire array
  }
  global_joiner.wait();

  // init Vertex[] 
  global_joiner.reset();
  if (nid==0) {
    async_parallel_for<safeinitVertex, joinerSpawn<safeinitVertex, VERIFY_THRESHOLD>, VERIFY_THRESHOLD >( 0, FLAGS_vertices_size ); // entire array
  }
  global_joiner.wait();
}

void safe_initialize_data() {
  safe_init_f sf;
  fork_join_custom(&sf);
}


// 
// Older tree search/gen implementations
//

/* **************************************************************************
 * Grappa ParallelForLoop implementation of tree search.
 * Not recently tested. Use with opt_ff=false
 * **************************************************************************/

// For children array loop iterations arguments for search
struct sibling_args_search {
  int64_t parentid;
  int64_t depth;
  GlobalAddress<Result> r;
  int64_t childid0;
};


// arguments to atomic max delegated function; TODO: this should be implemented
// as a generic delegate operation
struct atomic_max_args {
  GlobalAddress<Result> result_ptr;
  int64_t value;
};

// active message for atomic max
void atomic_max_am( atomic_max_args * args, size_t args_size, void * payload, size_t payload_size ) {
  Result * r = args->result_ptr.pointer();
  r->maxdepth =  ( args->value > r->maxdepth ) ? args->value : r->maxdepth;
}

/// Takes the max of the value at addr and value, atomically.
void atomic_max( GlobalAddress<Result> addr, int64_t value ) {
  atomic_max_args args = { addr, value };
  SoftXMT_call_on( addr.node(), &atomic_max_am, &args );
}


Result parTreeSearch(int64_t depth, int64_t id);

// Loop body for for loop over children array.
//
// part of less well tested ParallelForLoop impl
void explore_child (int64_t i, sibling_args_search * s) {
  int64_t id = s->childid0 + i;
  int64_t pid = s->parentid;

  // payload processing
  //for (i = 0; i < payloadSize; i++) Payload[(id*payloadSize+i)&0x3fffffff]+=Payload[(pid*payloadSize+i)&0x3fffffff];

  Result c = parTreeSearch(s->depth+1, id);

  // update the max depth of parent   
  atomic_max( s->r, c.maxdepth ); 

  // update the size and leaves
  SoftXMT_delegate_fetch_and_add_word( global_pointer_to_member( s->r, &Result::size ), c.size );
  SoftXMT_delegate_fetch_and_add_word( global_pointer_to_member(s->r, &Result::leaves), c.leaves );
}

/// Perform a parallel traversal of the tree starting at vertex id.
///
/// part of less well tested ParallelForLoop impl
Result parTreeSearch(int64_t depth, int64_t id) {

  Result r = { depth, 1, 0 };

  VLOG_EVERY_N(2, 250000) << "search vertex" << id;

  int64_t numChildren;
  int64_t childIndex;
  {
    vertex_t v_storage;
    Incoherent<vertex_t>::RO v( Vertex + id, 1, &v_storage );
    /* (v) = Vertex[id] */

    numChildren = (*v).numChildren;
    childIndex = (*v).childIndex;
  }

  int64_t childid0_val;
  {
    int64_t childid0_storage;
    Incoherent<int64_t>::RO childid0( Child + childIndex, 1, &childid0_storage ); // XXX: this is not edgelist acquire right now because there is locality in addresses of children (see explore child just does childid0+i)
    /** (childid0) = Child[ChildIndex[id]]; **/
    childid0_val = *childid0;
  }

  // Recurse on the children
  if (numChildren > 0) {
    sibling_args_search args = {id, depth, make_global( &r ), childid0_val}; 
    parallel_loop_implFuture(0, numChildren, &explore_child, args);
  } else {
    r.leaves = 1;
  }

  return r;
}


/* ****************************************************************************
 * Parallel Tree traversal that uses explicit counting to know when it is done.
 * Every Node knows how many tree vertices are stored locally, and when a task
 * visits a vertex it sends a count increment to the Node where the vertex is stored.
 * When a task visits a vertex, it spawns tasks serially to visit the children
 * and then exits.
 *
 * This is a less well tested implementation. Run with opt_ff = false.
 * ***************************************************************************/
  
void noJoinParTreeSearch(int64_t id);

void no_join_explore_child (int64_t id) {
  // payload processing
  //for (i = 0; i < payloadSize; i++) Payload[(id*payloadSize+i)&0x3fffffff]+=Payload[(pid*payloadSize+i)&0x3fffffff];

  VLOG_EVERY_N(2, 250000) << "explore child " << id;

  noJoinParTreeSearch( id );
}

// simply spawns all the iterations serially
void no_join_parallel_loop( int64_t start, int64_t iters, int64_t startId ) {
  for (int64_t i = start; i<iters; i++) {
    // uses 8-byte arg field only
    SoftXMT_publicTask( &no_join_explore_child, startId + i );
  }
}

void release_vertex_semaphore_am( int64_t * arg, size_t arg_size, void * payload, size_t payload_size ) {
  node_private_vertex_sem->release( 1 );
}

void release_vertex_semaphore( Node n ) {
  int64_t ignore = 0;
  if ( SoftXMT_mynode() == n ) {
    node_private_vertex_sem->release( 1 );
  } else {
    SoftXMT_call_on( n, &release_vertex_semaphore_am, &ignore);
  }
}

/// Implementation of parallel tree traversal with serially-spawned for-loop iterations
/// and explicit counting for termination.
void noJoinParTreeSearch(int64_t id) {
  int64_t numChildren;
  int64_t childIndex;

  GlobalAddress<vertex_t> v_addr = Vertex + id;
  { 
    vertex_t v_storage;
    Incoherent<vertex_t>::RO v( v_addr, 1, &v_storage );
    /* (v) = Vertex[id] */

    numChildren = (*v).numChildren;
    childIndex = (*v).childIndex;
  }

  int64_t childid0_val;
  {
    int64_t childid0_storage;
    Incoherent<int64_t>::RO childid0( Child + childIndex, 1, &childid0_storage ); // XXX: this is not edgelist acquire right now because there is locality in addresses of children (see explore child just does childid0+i)
    /** (childid0) = Child[ChildIndex[id]]; **/
    childid0_val = *childid0;
  }

  // Recurse on the children
  if (numChildren > 0) {
    no_join_parallel_loop(0, numChildren, childid0_val);
  }

  // count the vertex (for termination)
  VLOG(3) << "counting vertex from Node " << v_addr.node();
  release_vertex_semaphore( v_addr.node() );
}



/* ****************************************************************************
 * Tree generation.
 * This is essentially UTS but includes global memory references to allocate
 * array space and to store the discovered tree in memory.
 * ***************************************************************************/

// RNG spawn delegate mailbox TODO: use Grappa's Descriptor or generic delegate
struct memory_descriptor {
  Thread * t;
  GlobalAddress<struct state_t> address;
  struct state_t data;
  bool done;
};

// RNG spawn delegate operation reply args
struct spawn_rng_reply_args {
  GlobalAddress<memory_descriptor> descriptor;
};

// rng spawn reply, sets the result data from the spawn
static void spawn_rng_reply_am( spawn_rng_reply_args * args, size_t size, void * payload, size_t payload_size ) {
  assert( payload_size == sizeof( struct state_t ) );
  args->descriptor.pointer()->data = *(static_cast<struct state_t *>(payload));
  args->descriptor.pointer()->done = true;
  SoftXMT_wake( args->descriptor.pointer()->t );
}

struct spawn_rng_request_args {
  GlobalAddress<memory_descriptor> descriptor;
  GlobalAddress<uts::Node> address;
  int i;
};

void spawn_rng_request_am( spawn_rng_request_args * args, size_t args_size, void * payload, size_t size ) {
  uts::Node * parent = args->address.pointer();
  struct state_t data;
  for (int j=0; j<computeGranularity; j++) {
    rng_spawn(parent->state.state, data.state, args->i);
  }

  spawn_rng_reply_args reply_args = { args->descriptor }; 
  SoftXMT_call_on( args->descriptor.node(), &spawn_rng_reply_am, 
      &reply_args, sizeof(reply_args), 
      &data, sizeof(data) );
}

/// perform UTS rng_spawn on the parent in global memory
struct state_t spawn_rng_remote( GlobalAddress<uts::Node> parentAddress, int child_num ) {
  memory_descriptor md;
  md.address = parentAddress;
  memset(&md.data, 0, sizeof(struct state_t));
  md.done = false;
  md.t = CURRENT_THREAD;
  spawn_rng_request_args args;
  args.descriptor = make_global(&md);
  args.address = parentAddress;
  args.i = child_num;
  SoftXMT_call_on( parentAddress.node(), &spawn_rng_request_am, &args );

  while( !md.done ) { // TODO: this should be using higher level primitives like Descriptor
    SoftXMT_suspend();
  }

  return md.data;
}

// arguments to loop body of for loop over children
struct sibling_args {
  int64_t childType;
  counter_t parentHeight;
  GlobalAddress<uts::Node> parent;
  int64_t depth;
  GlobalAddress<Result> r;
  int64_t childid0;
};

Result parTreeCreate( int64_t depth, uts::Node * parent );

// loop body of for loop over children
// initialization of the child vertex, count the vertex, then visit it
void create_children( int64_t i, sibling_args * s ) {
  uts::Node child;
  child.type = s->childType;
  child.height = s->parentHeight + 1;
  child.numChildren = -1; // not yet determined
  child.id = s->childid0 + i;

  // count this vertex
  // TODO could do this in a subsequent kernel in streaming order
  SoftXMT_delegate_fetch_and_add_word( make_global( &node_private_num_vertices, (Vertex+child.id).node()), 1 );


  child.state = spawn_rng_remote( s->parent, i );
  // spawn child RNG state
  // TODO Coherent cache object (instead of AM)
  //    struct state_t parentState_storage;
  //    Incoherent<struct state_t>::RW parentState( getFieldAddress( s.parent, uts::Node, state ), 1, &parentState_storage );
  //    
  //    for (int j=0; j<computeGranularity; j++) {
  //        rng_spawn((*parentState).state, child.state.state, i);
  //    }
  //
  //    parentState.block_until_released();

  VLOG(5) << "Calling parTreeCreate on child " << child
    << "\n\t\tof iter=" << i << " of parentGA=" << s->parent;
  Result c = parTreeCreate(s->depth+1, &child);

  // update the max depth of parent   
  atomic_max( s->r, c.maxdepth ); 

  // update the size and leaves
  int64_t size_result = SoftXMT_delegate_fetch_and_add_word( global_pointer_to_member(s->r, &Result::size), c.size );
  int64_t leaves_result = SoftXMT_delegate_fetch_and_add_word( global_pointer_to_member(s->r, &Result::leaves), c.leaves );
  VLOG(4) << "added c.size=" << c.size << " to s->r->size=" << size_result;
  VLOG(4) << "added c.leaves=" << c.size << " to s->r->leaves=" << leaves_result;
}

/// Create a UTS tree recursively starting at the given root vertex
Result parTreeCreate( int64_t depth, uts::Node * parent ) {
  int64_t numChildren, childType;
  counter_t parentHeight = parent->height; 

  Result r = { depth, 1, 0 };

  // calculate the number of children of this vertex and the child type
  numChildren = uts_numChildren( parent );
  childType = uts_childType( parent );

  // for remembering the tree created
  int64_t id = uts_nodeId(parent);

  // Assign fresh unique ids for the children
  int64_t childid0 = SoftXMT_delegate_fetch_and_add_word( global_id_ga, numChildren );
  VLOG_EVERY_N(2, 250000) << "new childids: [" << childid0 << ", " << (childid0 + numChildren - 1) << "]";

  // Record ids and indices of the children
  int64_t index = SoftXMT_delegate_fetch_and_add_word( global_child_index_ga, numChildren );

  // Record the number and index of children
  VLOG(5) << "record vertex: " << (vertex_t){ numChildren, index } << "\n\t\tfor parent=" << *parent << "\n\t\tat " << make_global(parent);
  vertex_t childVertex_storage;
  Incoherent<vertex_t>::WO childVertex( Vertex + id, 1, &childVertex_storage );
  (*childVertex).numChildren = numChildren;
  (*childVertex).childIndex  = index;
  childVertex.block_until_released();

  // if the vertex has children, go visit them, otherwise can return
  if ( numChildren > 0 ) { 
    // Write out child indexes as one block
    // Incoherent acquire is fine because this section of Child is reserved
    int64_t childIds_storage[numChildren];
    Incoherent<int64_t>::WO childIds( Child + index, numChildren, childIds_storage );

    for (int i = 0; i < numChildren; i++) {
      VLOG(5) << "[in progress] writing out childIds [i=" << i << "]="<< childid0 + i << "\n\t\tfor parent=" << *parent;
      childIds[i] = childid0 + i;
    }
    childIds.block_until_released();
    VLOG(5) << "[done] writing out childIds [" << childid0 << "," << childid0 + (numChildren-1) << "]\n\t\tfor parent=" << *parent;
  } else {
    VLOG(5) << "[done] no childIds to write\n\t\tfor parent=" << *parent;
  }

  // Recurse on the children
  if (numChildren > 0) {
    sibling_args args = { childType, parentHeight, make_global( parent ), depth, make_global( &r ), childid0 };
    parallel_loop_implFuture(0, numChildren, &create_children, args);
  } else {
    r.leaves = 1;
  }

  return r;
}

// End older tree search/gen implementations




/* *****************************
 * User main program
* *****************************/

// utilities for ALLNODES operations

LOOP_FUNCTION(tree_search_semaphore_func, nid) {
    VLOG(2) << "Node " << nid << "will acquire_all";
    node_private_vertex_sem->acquire_all( CURRENT_THREAD );
}
LOOP_FUNCTION(initialize_tss_func, nid) {
    node_private_vertex_sem = new Semaphore( node_private_num_vertices, 0 );
    VLOG(2) << "Node " << nid << " has " << node_private_num_vertices << " vertices. New Sem="<<(void*)node_private_vertex_sem;
}

LOOP_FUNCTION(start_prof,nid) {
  SoftXMT_start_profiling();
}

LOOP_FUNCTION(stop_prof,nid) {
  SoftXMT_stop_profiling();
}
void start_profiling() {
  start_prof f;
  fork_join_custom(&f);
}
void stop_profiling() {
  stop_prof f;
  fork_join_custom(&f);
}

void par_create_tree() {
    create_func cfu;
    fork_join_custom(&cfu);
}
void par_search_tree(int64_t root) {
    search_func sfu;
    sfu.root = root;
    fork_join_custom(&sfu);
}


struct user_main_args {
    int argc;
    char** argv;
};


#define BILLION 1000000000

/// Grappa UTS-mem
void user_main ( user_main_args * args ) {

  // allocate tree structures 
  LOG(INFO) << "Allocating global structures...\n"
    << "\tVertex= " << ((double)sizeof(vertex_t)*FLAGS_vertices_size)/BILLION << "\n"
    << "\tChild= " << ((double)sizeof(int64_t)*FLAGS_vertices_size)/BILLION << "\n"
    << "\tTree_Nodes= "<< ((double)sizeof(uts::Node)*FLAGS_vertices_size)/BILLION << "\n"
    << "\tTotal= " << ((double)((sizeof(vertex_t)*FLAGS_vertices_size)
          +(sizeof(int64_t)*FLAGS_vertices_size)
          +(sizeof(uts::Node)*FLAGS_vertices_size)))/BILLION;

  Vertex = SoftXMT_typed_malloc<vertex_t>( FLAGS_vertices_size); 
  Child =  SoftXMT_typed_malloc<int64_t>( FLAGS_vertices_size);
  VLOG(2) << "Vertex = " << Vertex;
  VLOG(2) << "Child = " << Child;

  // allocate space for uts::Nodes just for the tree creation
  if ( opt_ff ) {
    Tree_Nodes = SoftXMT_typed_malloc<uts::Node>( FLAGS_vertices_size );
    VLOG(2) << "Tree_Nodes = " << Tree_Nodes;
  }

  // initialization on each node
  init_node_f infu;
  infu.Child_addr = Child;
  infu.Vertex_addr = Vertex;
  infu.Tree_Nodes_addr = Tree_Nodes;
  fork_join_custom(&infu);

  // initialization to support verification
  if ( FLAGS_verify_tree ) {
    LOG(INFO) << "initializing arrays to support verification";
    safe_initialize_data();
  }

  // print tree/search parameters
  uts_printParams();

  // initialize root of the tree
  uts::Node root;
  uts_initRoot(&root, type);

  // root
  global_id = 1;
  root.id = 0;

  // run times
  double t1=0.0, t2=0.0;

  SoftXMT_reset_stats_all_nodes();

  //
  // 1. start tree generation (traditional UTS with storing the tree)
  //
  LOG(INFO) << "starting tree generation";
  Result r_gen;
  //    start_profiling();
  t1 = uts_wctime();
  if ( opt_ff ) {
    
    par_create_tree();
    
    r_gen.maxdepth = 0; // don't care
    r_gen.leaves = 0; // don't care
    r_gen.size = -1; // will calculate with a reduce
  } else {
    r_gen = parTreeCreate(0, &root);
  }
  t2 = uts_wctime();
  //    stop_profiling();


  if ( opt_ff ) { 
    // count nodes generated
    r_gen.size = 0;
    for (Node n=0; n<SoftXMT_nodes(); n++) {
      uint64_t this_size = SoftXMT_delegate_read_word( make_global( &tj_num_gen_nodes, n ) );  
      LOG(INFO) << "Node " << n << " generated " << this_size;
      r_gen.size += this_size;
    }

    // only needed for generation
    SoftXMT_free( Tree_Nodes );
  }

  // show tree stats 
  counter_t maxTreeDepth = r_gen.maxdepth;
  counter_t nNodes  = r_gen.size;
  counter_t nLeaves = r_gen.leaves;

  double gen_runtime = t2-t1;

  uts_showStats(SoftXMT_nodes(), FLAGS_chunk_size, gen_runtime, nNodes, nLeaves, maxTreeDepth);


  //
  // 2. verify generated tree
  // 
  if ( FLAGS_verify_tree ) {
    LOG(INFO) << "starting tree verify";
    t1 = uts_wctime();

    verify_generation( r_gen.size );

    t2 = uts_wctime();
    double veri_runtime = t2-t1;

    LOG(INFO) << "verify runtime: " << veri_runtime << " s";
  }


  // TODO Payload =  SoftXMT_typed_malloc<int64_t>( FLAGS_vertices_size );
  // payload initialization on each node
  //payinit_f pfu;
  //pfu.Payload_ = Payload;
  //fork_join_custom(&pfu);


  //
  // 3. traverse the in-memory tree
  //
  LOG(INFO) << "starting tree search";
  Result r_search;
  start_profiling();
  SoftXMT_reset_stats_all_nodes();
  t1 = uts_wctime();
  if ( opt_ff ) {
    par_search_tree( 0 );

    r_search.maxdepth = 0; // don't care
    r_search.leaves = 0; // don't care
    r_search.size = -1; // will calculate with reduce
  } else {
    r_search =  parTreeSearch(0, 0);
  }
  t2 = uts_wctime();
  stop_profiling();
  SoftXMT_merge_and_dump_stats();

  if ( opt_ff ) { 
    // count nodes searched
    r_search.size = 0;
    for (Node n=0; n<SoftXMT_nodes(); n++) {
      uint64_t this_size = SoftXMT_delegate_read_word( make_global( &tj_num_searched_nodes, n ) );  
      LOG(INFO) << "Node " << n << " searched " << this_size;
      r_search.size += this_size;
    }
  }

  double search_runtime = t2-t1;

  // extra experimental search implementation
  if ( !opt_ff ) {
    LOG(INFO) << "starting simple no-join tree search";
    // no-join tree search
    // use one semaphore per node to count up vertices for termination
    initialize_tss_func itss_f;
    fork_join_custom( &itss_f );

    SoftXMT_reset_stats_all_nodes();

    start_profiling();
    t1 = uts_wctime();

    // start the tree search
    noJoinParTreeSearch( 0 );

    // join on all Nodes' vertex counts
    tree_search_semaphore_func tss_f;
    fork_join_custom( &tss_f );

    t2 = uts_wctime();
    stop_profiling();

    //SoftXMT_dump_task_series();
  }
  double noJoin_runtime = t2-t1;

  SoftXMT_free( Vertex );
  SoftXMT_free( Child );
  //TODO SoftXMT_free( Payload );


  LOG(INFO) << "generated size=" << r_gen.size << ", searched size=" << r_search.size;
  CHECK(r_gen.size == r_search.size);

  LOG(INFO) << "uts: {"
    << "search_runtime: " << search_runtime << ","
    //<< "noJoin_runtime: " << noJoin_runtime << ","
    << "nNodes: " << nNodes
    << "}";

  std::cout << "uts: {"
    << "search_runtime: " << search_runtime << ","
    //<< "noJoin_runtime: " << noJoin_runtime << ","
    << "nNodes: " << nNodes
    << "}" << std::endl;

  LOG(INFO) << ((double)nNodes / search_runtime) / 1000000 << " Mvert/s";
}


/// Main() entry
int main (int argc, char** argv) {
    SoftXMT_init( &argc, &argv ); 
    SoftXMT_activate();

    // TODO: would be good to give user interface to get the args as pass to this Node; to avoid this
    // (sometimes all nodes parse their own args instead of passing variable size argv)
    global_argc = argc;
    global_argv = argv;

    user_main_args uargs = { argc, argv };

    SoftXMT_run_user_main( &user_main, &uargs );
    CHECK( SoftXMT_done() == true ) << "SoftXMT not done before scheduler exit";
    SoftXMT_finish( 0 );
}
