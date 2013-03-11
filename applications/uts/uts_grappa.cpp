// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "uts.h"

#include <Grappa.hpp>
#include <Cache.hpp>
#include <ParallelLoop.hpp>
#include <GlobalAllocator.hpp>
#include <Addressing.hpp>
#include <PerformanceTools.hpp>
#include <GlobalCompletionEvent.hpp>
#include <Array.hpp>
#include <Statistics.hpp>


#include <iostream>

#include <stdio.h>

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

using namespace Grappa;

// UTS-mem application statistics
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, uts_num_gen_nodes, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, uts_num_searched_nodes, 0);
// for holding their final values
uint64_t local_searched;
uint64_t local_generated;


// Parallel granularities for important parallel for-loops
#define VERIFY_THRESHOLD ((int64_t) 4)
#define CREATE_THRESHOLD ((int64_t) 1)
// threshold for tree search is specified by Grappa option --async_par_for_threshold

// turn on for initiating streaming writes instead
// of waiting until the WO cache objects go out of scope
// Only reason to turn this off is if we find it causes extra
// context switching that is detrimental to performance.
#define TREEGEN_EAGER_RELEASE 1

// declare Grappa stealing parameters
DECLARE_string( load_balance );
DECLARE_int32( chunk_size );



/* ****************************************************************************
 *  UTS interface definitions
 * ****************************************************************************/
char * impl_getName() { return "Grappa"; }
int    impl_paramsToStr(char * strBuf, int ind) {
  ind += sprintf(strBuf+ind, "Execution strategy:  ");

  ind += sprintf(strBuf+ind, "Parallel search using %d processes\n", Grappa_nodes());
  ind += sprintf(strBuf+ind, "   up to %d threads per core\n", FLAGS_num_starting_workers );

  if ( FLAGS_load_balance.compare(        "none" ) != 0 ) {
    ind += sprintf(strBuf+ind, "    Dynamic load balance with chunk size = %d nodes\n", FLAGS_chunk_size);
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
  //Grappa_finish( err );//TODO KILL ALL NODES
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

/// verification counts
uint64_t local_verify_children_count = 0;

// node-local client to loop syncrhonization
GlobalCompletionEvent joiner;
GlobalCompletionEvent s_joiner;  //FIXME: different joiner for debugging (use each once)

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
//TODO GlobalAddress<int64_t> Payload;

/// Store results of tree traversals
struct Result {
  int64_t maxdepth, size, leaves; 
};


/* ************************************************************
 * GlobalTaskJoiner feed forward versions of create and search
 *
 * ***********************************************************/

void tj_create_children( uts::Node * parent );

// loop body (ranged for caching) of for loop that creates all the new children
void tj_create_vertex( int64_t start, int64_t num, int64_t parent_id ) {

  // acquiring these two parent items is the only thing shared by doing
  // num > 1 iterations

  uts::Node parent_storage;
  Incoherent<uts::Node>::RO parentc( Tree_Nodes + parent_id, 1, &parent_storage);

  // TODO: this is a lot of extra reads just to obtain childid0; easier to have args but feed forward..
  int64_t p_childIndex = Grappa::delegate::read( global_pointer_to_member( Vertex+parent_id, &vertex_t::childIndex) );
  int64_t p_childid0 = Grappa::delegate::read( Child + p_childIndex );

  parentc.block_until_acquired();
  int64_t childType = uts_childType(&parent_storage);// need to pass in normal pointer, so have to call block_until_acquired explicitly
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
  uts_num_gen_nodes++;

  DVLOG(5) << "creating children for id=" << parent->id;
  Incoherent<uts::Node>::WO v( Tree_Nodes + parent->id, 1, parent );

  int64_t numChildren = uts_numChildren( parent );
  (*v).numChildren = numChildren;
#if TREEGEN_EAGER_RELEASE
  v.start_release(); // done writing to v locally
#endif

  /* Assign fresh unique ids for the children: */
  /* TODO: make global_id_ga not a hotspot */
  int64_t childid0 = Grappa::delegate::fetch_and_add( global_id_ga, numChildren );
  VLOG_EVERY_N(2, 250000) << "new childids: [" << childid0 
    << ", " << (childid0 + numChildren - 1) 
    << "]";

  /* Record ids and indices of the children: */ 
  /* TODO: make global_child_index_ga not a hotspot */
  int64_t index = Grappa::delegate::fetch_and_add( global_child_index_ga, numChildren );

  /* store parts of parent that will be read in tree search */
  vertex_t vvert_storage;
  Incoherent<vertex_t>::WO vvert( Vertex + parent->id, 1, &vvert_storage );
  (*vvert).numChildren = numChildren;
  (*vvert).childIndex = index;
#if TREEGEN_EAGER_RELEASE
  vvert.start_release();  // done writing to vvert locally
#endif

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

  int64_t parentID = parent->id;
  forall_here_async_public< &joiner >( 0, numChildren, [parentID]( int64_t start, int64_t iters ) {
    tj_create_vertex( start, iters, parentID );
  });   
}





///
/// Traverse the in-memory tree starting from Vertex[start]
///
void search_vertex( int64_t id ) {
  uts_num_searched_nodes++;
  int64_t numChildren, childIndex;

  GlobalAddress<vertex_t> v_addr = Vertex + id;
  { 
    vertex_t v_storage;
    Incoherent<vertex_t>::RO v( v_addr, 1, &v_storage );
    /* (v) = Vertex[id] */

    DVLOG(5) << "Search vertex " << *v << "(local count = " << uts_num_searched_nodes << ")";

    numChildren = (*v).numChildren;
    childIndex = (*v).childIndex;
  }

  // iterate over my children 
  forall_here_async_public< &s_joiner >( childIndex, numChildren, []( int64_t start, int64_t iters ) {
      int64_t c0;
      {
        // all iterations in the range share the relevant chunk of the child list
        int64_t childid_storage[iters];
        Incoherent<int64_t>::RO childids( Child + start, iters, childid_storage ); // this will cause O(Branching factor * depth) space

        // spawn tasks serially for the first nc-1 chilren
        for ( int64_t i=0; i<iters-1; i++ ) {
          int64_t c = childids[i];
          publicTask<&s_joiner>( [c]() {
            search_vertex( c );
          });
        }
        if (iters>0) {
          c0 = childids[iters-1];
        }
      } // release the cache object so we don't hold on to it (despite tail recursion compiler might not call destructor)

      if (iters>0) {
        // explore the nc child myself
        search_vertex( c0 );
      }
  });
}


///////////////////////////////////////////////////////////////
// Tree generation verification
///////////////////////////////////////////////////////////////

void verify_generation( uint64_t num_vert ) {
  // do checks on child array
  forall_localized(Child, num_vert-1, [](int64_t i, int64_t& c) {
    CHECK( c > 0 ) << "Child[" << i << "] = " << c; // > 0 because root is never c
  });

  // do checks on vertex_t array
  forall_localized(Vertex, num_vert, [](int64_t i, vertex_t& v) {
    CHECK( v.numChildren >= 0 ) << "Vertex[" << i << "].numChildren = " << v.numChildren;
    CHECK( v.childIndex >= 0 )  << "Vertex[" << i << "].childIndex = "  << v.childIndex;

    local_verify_children_count += v.numChildren;
  });

  // count numChildren entries
  uint64_t total_numChildren = Grappa::reduce<uint64_t, collective_add<uint64_t> >( &local_verify_children_count );

  CHECK( total_numChildren == num_vert-1 ) << "verify got " << total_numChildren <<", expected " << num_vert-1;
}


void safe_initialize_data() {
  vertex_t reset_vertex;
  reset_vertex.numChildren = -2;
  reset_vertex.childIndex  = -3;
  Grappa_memset_local( Child, -1L, FLAGS_vertices_size );
  Grappa_memset_local( Vertex, reset_vertex, FLAGS_vertices_size );
}


/* ****************************************************************************
 * Tree generation.
 * This is essentially UTS but includes global memory references to allocate
 * array space and to store the discovered tree in memory.
 * ***************************************************************************/



/* *****************************
 * User main program
* *****************************/

// utilities for ALLNODES operations
void start_profiling() {
  on_all_cores( [] {
    Grappa_start_profiling();
  });
}

void stop_profiling() {
  on_all_cores( [] {
    Grappa_stop_profiling();
  });
}

void par_create_tree() {
  global_id = 0;
  uts::Node root;
  uts_initRoot( &root, type );
  root.id = global_id++;
  tj_create_children( &root );
  joiner.wait();
}

void par_search_tree(int64_t root) {
  DVLOG(5) << "starting search";
  search_vertex( root );
  s_joiner.wait();
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

  GlobalAddress<vertex_t> Vertex_l = Grappa_typed_malloc<vertex_t>( FLAGS_vertices_size); 
  GlobalAddress<int64_t> Child_l =  Grappa_typed_malloc<int64_t>( FLAGS_vertices_size);
  VLOG(2) << "Vertex = " << Vertex_l;
  VLOG(2) << "Child = " << Child_l;

  // allocate space for uts::Nodes just for the tree creation
  GlobalAddress<uts::Node> Tree_Nodes_l;
  Tree_Nodes_l = Grappa_typed_malloc<uts::Node>( FLAGS_vertices_size );
  VLOG(2) << "Tree_Nodes = " << Tree_Nodes_l;

  // initialization on each node
  on_all_cores( [Child_l, Vertex_l, Tree_Nodes_l] {
    global_id_ga = make_global( &global_id, 0 );
    global_child_index_ga = make_global( &global_child_index, 0 );

    Child = Child_l;
    Vertex = Vertex_l;
    Tree_Nodes = Tree_Nodes_l;

    // initialize UTS with args
    LOG(INFO) << "Initializing UTS";
    uts_parseParams(global_argc, global_argv);
  });

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

  Grappa_reset_stats_all_nodes();

  //
  // 1. start tree generation (traditional UTS with storing the tree)
  //
  LOG(INFO) << "starting tree generation";
  Result r_gen;
  //    start_profiling();
  t1 = uts_wctime();

  par_create_tree();

  r_gen.maxdepth = 0; // don't care
  r_gen.leaves = 0; // don't care
  r_gen.size = -1; // will calculate with a reduce

  t2 = uts_wctime();
  //    stop_profiling();


  // count nodes generated
  on_all_cores( [] {
    local_generated = uts_num_gen_nodes.value();
    LOG(INFO) << "Core " << Grappa::mycore() << " generated " << local_generated;
  });
  r_gen.size = Grappa::reduce< uint64_t, collective_add<uint64_t> >( &local_generated );
  LOG(INFO) << "Total generated: " << r_gen.size;

  // only needed for generation
  Grappa_free( Tree_Nodes );

  // show tree stats 
  counter_t maxTreeDepth = r_gen.maxdepth;
  counter_t nNodes  = r_gen.size;
  counter_t nLeaves = r_gen.leaves;

  double gen_runtime = t2-t1;

  uts_showStats(Grappa::cores(), FLAGS_chunk_size, gen_runtime, nNodes, nLeaves, maxTreeDepth);


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
  } else {
    LOG(INFO) <<  "WARNING: not verifying generated tree";
  }


  // TODO Payload =  Grappa_typed_malloc<int64_t>( FLAGS_vertices_size );
  // on_all_cores([Payload_addr] {
  //  Payload = Payload_addr;
  // });
  // payload initialization on each node


  //
  // 3. traverse the in-memory tree
  //
  LOG(INFO) << "starting tree search";
  Result r_search;
  //start_profiling();
  on_all_cores( [] { Grappa::Statistics::reset(); } );
  t1 = uts_wctime();
 
  par_search_tree( 0 );

  r_search.maxdepth = 0; // don't care
  r_search.leaves = 0; // don't care
  r_search.size = -1; // will calculate with reduce
  
  t2 = uts_wctime();
  //stop_profiling();

  Grappa::Statistics::merge_and_print();

  // count nodes searched
  on_all_cores( [] {
    local_searched = uts_num_searched_nodes.value();
    LOG(INFO) << "Node " << Grappa::mycore() << " searched " << local_searched;
  });
  r_search.size = Grappa::reduce< uint64_t, collective_add<uint64_t> >( &local_searched );

  double search_runtime = t2-t1;


  Grappa_free( Vertex );
  Grappa_free( Child );
  //TODO Grappa_free( Payload );


  LOG(INFO) << "generated size=" << r_gen.size << ", searched size=" << r_search.size;
  CHECK(r_gen.size == r_search.size);

  LOG(INFO) << "uts: {"
    << "search_runtime: " << search_runtime << ","
    << "nNodes: " << nNodes
    << "}";

  std::cout << "uts: {"
    << "search_runtime: " << search_runtime << ","
    << "nNodes: " << nNodes
    << "}" << std::endl;

  LOG(INFO) << ((double)nNodes / search_runtime) / 1000000 << " Mvert/s";
}


/// Main() entry
int main (int argc, char** argv) {
    Grappa_init( &argc, &argv ); 
    Grappa_activate();

    // TODO: would be good to give user interface to get the args as pass to this Node; to avoid this
    // (sometimes all nodes parse their own args instead of passing variable size argv)
    global_argc = argc;
    global_argv = argv;

    user_main_args uargs = { argc, argv };

    Grappa_run_user_main( &user_main, &uargs );
    CHECK( Grappa_done() == true ) << "Grappa not done before scheduler exit";
    Grappa_finish( 0 );
}
