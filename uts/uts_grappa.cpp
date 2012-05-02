#include "uts.h"

#include <SoftXMT.hpp>
#include <Cache.hpp>
#include <ParallelLoop.hpp>
#include <ForkJoin.hpp>
#include <GlobalAllocator.hpp>
#include <Addressing.hpp>

#include <iostream>

#include <stdlib.h> // for memset

DEFINE_int64( num_places, 2, "Number of locality domains this is running on (e.g., machines, 'locales')" );
DEFINE_int64( vertices_size, 1<<20, "Upper bound count of vertices" );

// declare stealing parameters
DECLARE_bool( steal );
DECLARE_int32( chunk_size );
DECLARE_int32( cancel_interval );

// UTS interface definitions
char * impl_getName() { return "Grappa"; }
int    impl_paramsToStr(char * strBuf, int ind) {
    ind += sprintf(strBuf+ind, "Execution strategy:  ");
    
    ind += sprintf(strBuf+ind, "Parallel search using %d processes\n", SoftXMT_nodes());
    ind += sprintf(strBuf+ind, "   up to %d threads per core\n", FLAGS_num_starting_workers );
    ind += sprintf(strBuf+ind, "%d places\n", FLAGS_num_places);
    
    if (FLAGS_steal) {
        ind += sprintf(strBuf+ind, "    Dynamic load balance by work stealing, chunk size = %d nodes\n", FLAGS_chunk_size);
        ind += sprintf(strBuf+ind, "  CBarrier Interval: %d\n", FLAGS_cancel_interval);
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
    // not using this mechanism for implementation params (gflags)
    return 1;
}

void impl_abort(int err) {
    exit(err);
    //SoftXMT_finish( err );//TODO KILL ALL NODES
} 

int global_argc;
char **global_argv;

uint64_t global_id; // only Node 0 matters
uint64_t global_child_index = 0; // only Node 0 matters
GlobalAddress<uint64_t> global_id_ga;
GlobalAddress<uint64_t> global_child_index_ga;

// these are for doing a join on all
// vertices being visited in the noJoin search
uint64_t node_private_num_vertices = 0;
Semaphore * node_private_vertex_sem;

// shared tree structures
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

GlobalAddress<vertex_t> Vertex;
GlobalAddress<int64_t> Child;
GlobalAddress<int64_t> Payload;

// Initialization on each node
struct init_args {
    GlobalAddress<Semaphore> sem;
    GlobalAddress<vertex_t> Vertex;
    GlobalAddress<int64_t> Child;
};

void init_node( init_args * args ) {
    global_id_ga = make_global( &global_id, 0 );
    global_child_index_ga = make_global( &global_child_index, 0 );

    Vertex = args->Vertex;
    Child = args->Child;
    
    // initialize UTS with args
    LOG(INFO) << "Initializing UTS";
    uts_parseParams(global_argc, global_argv);

    Semaphore::release( &args->sem, 1 );
}
DECLARE_CACHE_WRAPPED(init_node_Caching, &init_node, init_args);

struct payinit_args {
    GlobalAddress<Semaphore> sem;
    GlobalAddress<int64_t> Payload;
};

void payinit( payinit_args * args ) {
    Payload = args->Payload;

    Semaphore::release( &args->sem, 1 );
}
DECLARE_CACHE_WRAPPED(payinit_Caching, &payinit, payinit_args);

//////////////////////////////////

struct Result {
  int64_t maxdepth, size, leaves; 
};

struct sibling_args {
  int64_t childType;
  counter_t parentHeight;
  GlobalAddress<uts::Node> parent;
  int64_t depth;
  GlobalAddress<Result> r;
  int64_t childid0;
};

struct sibling_args_search {
  int64_t parentid;
  int64_t depth;
  GlobalAddress<Result> r;
  int64_t childid0;
};


//////////////////////
// Atomic max function
struct atomic_max_args {
    GlobalAddress<Result> result_ptr;
    int64_t value;
};

void atomic_max_am( atomic_max_args * args, size_t args_size, void * payload, size_t payload_size ) {
    Result * r = args->result_ptr.pointer();
    r->maxdepth =  ( args->value > r->maxdepth ) ? args->value : r->maxdepth;
}

void atomic_max( GlobalAddress<Result> addr, int64_t value ) {
    atomic_max_args args = { addr, value };
    SoftXMT_call_on( addr.node(), &atomic_max_am, &args );
}
////////////////////


Result parTreeSearch(int64_t depth, int64_t id);

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
        parallel_loop(0, numChildren, &explore_child, args);
    } else {
        r.leaves = 1;
    }

    return r;
}

///
/// tail recursive parallel tree search
///
void noJoinParTreeSearch(int64_t id);

void no_join_explore_child (int64_t id) {
  // payload processing
  //for (i = 0; i < payloadSize; i++) Payload[(id*payloadSize+i)&0x3fffffff]+=Payload[(pid*payloadSize+i)&0x3fffffff];
  
  VLOG_EVERY_N(2, 250000) << "explore child " << id;
  
  noJoinParTreeSearch( id );
}

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



//////////////////////////////////////////////////
// RNG spawn delegate operation
struct memory_descriptor {
  Thread * t;
  GlobalAddress<struct state_t> address;
  struct state_t data;
  bool done;
};

struct spawn_rng_reply_args {
  GlobalAddress<memory_descriptor> descriptor;
};

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
  
  while( !md.done ) {
    SoftXMT_suspend();
  }
  
  return md.data;
}
///////////////////////////////////////////////////

Result parTreeCreate( int64_t depth, uts::Node * parent );

void create_children( int64_t i, sibling_args * s ) {
    uts::Node child;
    child.type = s->childType;
    child.height = s->parentHeight + 1;
    child.numChildren = -1; // not yet determined
    child.id = s->childid0 + i;

    // count this vertex
    // TODO could do this offline in streaming order
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
   
int64_t uts_nodeId( uts::Node * n ) {
    return n->id;
}

Result parTreeCreate( int64_t depth, uts::Node * parent ) {
    int64_t numChildren, childType;
    counter_t parentHeight = parent->height; 
    
    Result r = { depth, 1, 0 };
    numChildren = uts_numChildren( parent );
    childType = uts_childType( parent );

    /*** Added for the sake of remembering the tree created: ***/
    int64_t id = uts_nodeId(parent);
    /* Assign fresh unique ids for the children: */
    int64_t childid0 = SoftXMT_delegate_fetch_and_add_word( global_id_ga, numChildren );
    VLOG_EVERY_N(2, 250000) << "new childids: [" << childid0 << ", " << (childid0 + numChildren - 1) << "]";
    /* Record ids and indices of the children: */ 
    int64_t index = SoftXMT_delegate_fetch_and_add_word( global_child_index_ga, numChildren );

    /* Record the number and index of children: */
    // TODO! Change to just a write (not R/W)
    VLOG(5) << "record vertex: " << (vertex_t){ numChildren, index } << "\n\t\tfor parent=" << *parent << "\n\t\tat " << make_global(parent);
    vertex_t childVertex_storage;
    Incoherent<vertex_t>::RW childVertex( Vertex + id, 1, &childVertex_storage );
    (*childVertex).numChildren = numChildren;
    (*childVertex).childIndex  = index;
    childVertex.block_until_released();
    //SoftXMT_delegate_write_word( NumChildren + id, numChildren );
    //SoftXMT_delegate_write_word( ChildIndex + id, index );
   

    if ( numChildren > 0 ) { 
        // Write out child indexes as one block
        // Incoherent acquire is fine because this section of Child is reserved
        int64_t childIds_storage[numChildren];
        Incoherent<int64_t>::RW childIds( Child + index, numChildren, childIds_storage );

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
        parallel_loop(0, numChildren, &create_children, args);
    } else {
        r.leaves = 1;
    }

    return r;
}

///
/// User main program
///
LOOP_FUNCTION(tree_search_semaphore_func, nid) {
    VLOG(2) << "Node " << nid << "will acquire_all";
    node_private_vertex_sem->acquire_all( CURRENT_THREAD );
}
LOOP_FUNCTION(initialize_tss_func, nid) {
    node_private_vertex_sem = new Semaphore( node_private_num_vertices, 0 );
    VLOG(2) << "Node " << nid << " has " << node_private_num_vertices << " vertices. New Sem="<<(void*)node_private_vertex_sem;
}

struct user_main_args {
    int argc;
    char** argv;
};

void user_main ( user_main_args * args ) {
  
    // allocate tree structures 
    Vertex = SoftXMT_typed_malloc<vertex_t>( FLAGS_vertices_size); 
    Child =  SoftXMT_typed_malloc<int64_t>( FLAGS_vertices_size);

    // initialization on each node
    Semaphore init_sem( SoftXMT_nodes(), 0 );
    init_args iargs = { make_global(&init_sem, 0),
                        Vertex,
                        Child };
    for (Node nod=0; nod<SoftXMT_nodes(); nod++) {
        SoftXMT_remote_privateTask( &init_node_Caching, make_global(&iargs), nod );
    }
    init_sem.acquire_all( CURRENT_THREAD );
    
    // print tree/search parameters
    uts_printParams();

    // initialize root of the tree
    uts::Node root;
    uts_initRoot(&root, type);
    
    global_id = 1;
    root.id = 0;

    // run times
    double t1=0.0, t2=0.0;
    
    SoftXMT_reset_stats_all_nodes();
   
    // start tree generation (traditional UTS, plus saving the tree)
    LOG(INFO) << "starting tree generation";
    t1 = uts_wctime();
    Result r_gen = parTreeCreate(0, &root);
    t2 = uts_wctime();
  
    SoftXMT_dump_task_series();
   
    // show tree stats 
    counter_t maxTreeDepth = r_gen.maxdepth;
    counter_t nNodes  = r_gen.size;
    counter_t nLeaves = r_gen.leaves;
  
    double gen_runtime = t2-t1;

    uts_showStats(SoftXMT_nodes(), FLAGS_chunk_size, gen_runtime, nNodes, nLeaves, maxTreeDepth);
   
    
    // TODO Payload =  SoftXMT_typed_malloc<int64_t>( FLAGS_vertices_size );
    // payload initialization on each node
    Semaphore pi_sem( SoftXMT_nodes(), 0 );
    payinit_args piargs = { make_global(&pi_sem, 0), Payload };
    for (Node nod=0; nod<SoftXMT_nodes(); nod++) {
        SoftXMT_remote_privateTask( &payinit_Caching, make_global(&piargs), nod );
    }
    init_sem.acquire_all( CURRENT_THREAD );
    
    SoftXMT_reset_stats_all_nodes();

    LOG(INFO) << "starting tree search";
    t1 = uts_wctime();
    Result r_search =  parTreeSearch(0, 0);
    t2 = uts_wctime();
    
    SoftXMT_dump_task_series();
    
    double search_runtime = t2-t1;

  
    // no-join tree search
    // use one semaphore per node to count up vertices for termination
    initialize_tss_func itss_f;
    fork_join_custom( &itss_f );
    
    SoftXMT_reset_stats_all_nodes();

    t1 = uts_wctime();

    // start the tree search
    noJoinParTreeSearch( 0 );

    // join on all Nodes' vertex counts
    tree_search_semaphore_func tss_f;
    fork_join_custom( &tss_f );
    
    t2 = uts_wctime();

    SoftXMT_dump_task_series();
    
    double noJoin_runtime = t2-t1;
   

    SoftXMT_free( Vertex );
    SoftXMT_free( Child );
    //TODO SoftXMT_free( Payload );
    
    
    LOG(INFO) << "generated size=" << r_gen.size << ", searched size=" << r_search.size;
    
    LOG(INFO) << "{"
              << "search_runtime: " << search_runtime << ","
              << "noJoin_runtime: " << noJoin_runtime << ","
              << "nNodes: " << nNodes
              << "}";
}

   

/// Main() entry
int main (int argc, char** argv) {
    SoftXMT_init( &argc, &argv, (1<<28) * (sizeof(int64_t) + sizeof(int64_t) + sizeof(vertex_t)) );
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
