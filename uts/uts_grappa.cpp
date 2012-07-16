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
DEFINE_int64( vertices_size, 4200000, "Upper bound count of vertices" );

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
    return 0;
}

void impl_abort(int err) {
    exit(err);
    //SoftXMT_finish( err );//TODO KILL ALL NODES
} 


//convenience
//calculate global address of a struct field from struct address (only use on POD)
#include <stddef.h>
#define getFieldAddress(resultAddr, objtype, field) make_global((resultAddr).pointer()+offsetof(objtype, field), (resultAddr).node())

uint64_t global_id; // only Node 0 matters
uint64_t global_child_index = 0; // only Node 0 matters
GlobalAddress<uint64_t> global_id_ga;
GlobalAddress<uint64_t> global_child_index_ga;

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
std::ostream& operator<< ( std::ostream& out, const TreeNode& n ) {
    out << "TreeNode{type=" << n.type
        << ", height=" << n.height
        << ", numChildren=" << n.numChildren
        << ", id=" << n.id
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
    int argc;
    char** argv;
};

void init_node( init_args * args ) {
    global_id_ga = make_global( &global_id, 0 );
    global_child_index_ga = make_global( &global_child_index, 0 );

    Vertex = args->Vertex;
    Child = args->Child;
    
    // initialize UTS with args
    uts_parseParams(args->argc, args->argv);

    Semaphore::release( &args->sem, 1 );
}

struct payinit_args {
    GlobalAddress<Semaphore> sem;
    GlobalAddress<int64_t> Payload;
};

void payinit( payinit_args * args ) {
    Payload = args->Payload;

    Semaphore::release( &args->sem, 1 );
}
//////////////////////////////////

struct Result {
  int64_t maxdepth, size, leaves; 
};

struct sibling_args {
  int64_t childType;
  counter_t parentHeight;
  GlobalAddress<TreeNode> parent;
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
  //TODO PAYLOAD for (i = 0; i < payloadSize; i++) Payload[(id*payloadSize+i)&0x3fffffff]+=Payload[(pid*payloadSize+i)&0x3fffffff];
  
  Result c = parTreeSearch(s->depth+1, id);
 
  // update the max depth of parent   
  atomic_max( s->r, c.maxdepth ); 

  // update the size and leaves
  SoftXMT_delegate_fetch_and_add_word( global_pointer_to_member( s->r, &Result::size ), c.size );
  SoftXMT_delegate_fetch_and_add_word( global_pointer_to_member(s->r, &Result::leaves), c.leaves );
}


Result parTreeSearch(int64_t depth, int64_t id) {

    Result r = { depth, 1, 0 };

    vertex_t v_storage;
    Incoherent<vertex_t>::RO v( Vertex + id, 1, &v_storage );
    /* (v) = Vertex[id] */

    int64_t numChildren = (*v).numChildren;
    int64_t childIndex = (*v).childIndex;

    int64_t childid0_storage;
    Incoherent<int64_t>::RO childid0( Child + childIndex, 1, &childid0_storage ); // XXX: this is not edgelist acquire right now because there is locality in addresses of children (see explore child just does childid0+i)
    /* (childid0) = Child[ChildIndex[id]]; */
    int64_t childid0_val = *childid0;
    childid0.block_until_released();

    // Recurse on the children
    if (numChildren > 0) {
        sibling_args_search args = {id, depth, make_global( &r ), childid0_val}; 
        parallel_loop(0, numChildren, &explore_child, args);
    } else {
        r.leaves = 1;
    }

    return r;
}


Result parTreeCreate( int64_t depth, TreeNode * parent );

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
    GlobalAddress<TreeNode> address;
};

void spawn_rng_request_am( spawn_rng_request_args * args, size_t args_size, void * payload, size_t size ) {
    TreeNode * parent = args->address.pointer();
    struct state_t data;
    for (int j=0; j<computeGranularity; j++) {
        rng_spawn(parent->state.state, data.state, j);
    }
   
    spawn_rng_reply_args reply_args = { args->descriptor }; 
    SoftXMT_call_on( args->descriptor.node(), &spawn_rng_reply_am, 
                   &reply_args, sizeof(reply_args), 
                   &data, sizeof(data) );
}

struct state_t spawn_rng_remote( GlobalAddress<TreeNode> parentAddress ) {
  memory_descriptor md;
  md.address = parentAddress;
  memset(&md.data, 0, sizeof(struct state_t));
  md.done = false;
  md.t = CURRENT_THREAD;
  spawn_rng_request_args args;
  args.descriptor = make_global(&md);
  args.address = parentAddress;
  SoftXMT_call_on( parentAddress.node(), &spawn_rng_request_am, &args );
  
  while( !md.done ) {
    SoftXMT_suspend();
  }
  
  return md.data;
}
///////////////////////////////////////////////////


void create_children( int64_t i, sibling_args * s ) {
    TreeNode child;
    child.type = s->childType;
    child.height = s->parentHeight + 1;
    child.numChildren = -1; // not yet determined
    child.id = s->childid0 + i;

    child.state = spawn_rng_remote( s->parent );
    // spawn child RNG state
    // TODO Coherent cache object (instead of AM)
//    struct state_t parentState_storage;
//    Incoherent<struct state_t>::RW parentState( getFieldAddress( s.parent, TreeNode, state ), 1, &parentState_storage );
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
    int64_t size_result = SoftXMT_delegate_fetch_and_add_word( getFieldAddress(s->r, Result, size), c.size );
    int64_t leaves_result = SoftXMT_delegate_fetch_and_add_word( getFieldAddress(s->r, Result, leaves), c.leaves );
    VLOG(4) << "added c.size=" << c.size << " to s->r->size=" << size_result;
    VLOG(4) << "added c.leaves=" << c.size << " to s->r->leaves=" << leaves_result;
}
   
int64_t uts_nodeId( TreeNode * n ) {
    return n->id;
}

Result parTreeCreate( int64_t depth, TreeNode * parent ) {
    int64_t numChildren, childType;
    counter_t parentHeight = parent->height; 
    
    Result r = { depth, 1, 0 };
    numChildren = uts_numChildren( parent );
    childType = uts_childType( parent );

    /*** Added for the sake of remembering the tree created: ***/
    int64_t id = uts_nodeId(parent);
    /* Assign fresh unique ids for the children: */
    int64_t childid0 = SoftXMT_delegate_fetch_and_add_word( global_id_ga, numChildren );
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
                        Child,
                        args->argc,
                        args->argv };
    for (Node nod=0; nod<SoftXMT_nodes(); nod++) {
        SoftXMT_remote_privateTask( &init_node, &iargs, nod );
    }
    init_sem.acquire_all( CURRENT_THREAD );
    
    // print tree/search parameters
    uts_printParams();

    // initialize root of the tree
    TreeNode root;
    uts_initRoot(&root, type);
    
    global_id = 1;
    root.id = 0;

    // run times
    double t1=0.0, t2=0.0;
   
    // start tree generation (traditional UTS, plus saving the tree)
    LOG(INFO) << "starting tree generation";
    t1 = uts_wctime();
    Result r = parTreeCreate(0, &root);
    t2 = uts_wctime();
   
    // show tree stats 
    counter_t maxTreeDepth = r.maxdepth;
    counter_t nNodes  = r.size;
    counter_t nLeaves = r.leaves;
  
    double gen_runtime = t2-t1;

    uts_showStats(SoftXMT_nodes(), FLAGS_chunk_size, gen_runtime, nNodes, nLeaves, maxTreeDepth);
   
    
    // TODO Payload =  SoftXMT_typed_malloc<int64_t>( FLAGS_vertices_size );
    // payload initialization on each node
    Semaphore pi_sem( SoftXMT_nodes(), 0 );
    payinit_args piargs = { make_global(&pi_sem, 0), Payload };
    for (Node nod=0; nod<SoftXMT_nodes(); nod++) {
        SoftXMT_remote_privateTask( &payinit, &piargs, nod );
    }
    init_sem.acquire_all( CURRENT_THREAD );

    LOG(INFO) << "starting tree search";
    t1 = uts_wctime();
    r =  parTreeSearch(0, 0);
    t2 = uts_wctime();
    
    double search_runtime = t2-t1;

    SoftXMT_free( Vertex );
    SoftXMT_free( Child );
    //TODO SoftXMT_free( Payload );
    
    LOG(INFO) << "{"
              << "runtime: " << search_runtime << ","
              << "nNodes: " << nNodes
              << "}";
}

   
size_t local_size_bytes = 1<<30; 

/// Main() entry
int main (int argc, char** argv) {
    SoftXMT_init( &argc, &argv, local_size_bytes*2 ); //TODO find num nodes (not 2)...
    SoftXMT_activate();

    user_main_args uargs = { argc, argv };

    SoftXMT_run_user_main( &user_main, &uargs );
    CHECK( SoftXMT_done() == true ) << "SoftXMT not done before scheduler exit";
    SoftXMT_finish( 0 );
}
