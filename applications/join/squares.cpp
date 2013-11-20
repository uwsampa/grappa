/*
 * Example of:
 * (partial) triangle listing
 *
 * A(x,z) :- R(x,y), R(y,z), R(z,x)
 */


// util includes
#include "Tuple.hpp"
#include "relation_IO.hpp"

// graph includes
#include "../graph500/grappa/graph.hpp"
#include "../graph500/generator/make_graph.h"
#include "../graph500/generator/utils.h"
#include "../graph500/prng.h"

#include "../graph500/grappa/graph.hpp"


// Grappa includes
#include <Grappa.hpp>
#include "MatchesDHT.hpp"
#include <Cache.hpp>
#include <ParallelLoop.hpp>
#include <GlobalCompletionEvent.hpp>
#include <AsyncDelegate.hpp>
#include <Statistics.hpp>
#include <FullEmpty.hpp>

// command line parameters

// file input
//DEFINE_string( fin, "", "Input file relation" );
//DEFINE_uint64( file_num_tuples, 0, "Number of lines in file" );

// generating input data
DEFINE_uint64( scale, 7, "Log of number of vertices" );
DEFINE_uint64( edgefactor, 16, "Median degree to try to generate" );
//DEFINE_bool( undirected, false, "Generated graph implies undirected edges" );

DEFINE_bool( print, false, "Print results" );


// outputs
GRAPPA_DEFINE_STAT(SimpleStatistic<double>, query_runtime, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, ir1_count, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, ir2_count, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, ir3_count, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, ir4_count, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, ir5_count, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, ir6_count, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, ir7_count, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, results_count, 0);

GRAPPA_DEFINE_STAT(SimpleStatistic<double>, index_runtime, 0);

GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, edges_transfered, 0);


using namespace Grappa;

std::ostream& operator<<( std::ostream& o, const Tuple& t ) {
  o << "T( ";
  for (uint64_t i=0; i<TUPLE_LEN; i++) {
    o << t.columns[i] << ", ";
  }
  o << ")";
}


template< typename T >
inline void print_array(const char * name, GlobalAddress<T> base, size_t nelem, int width = 10, int64_t limit=-1) {
  std::stringstream ss; 
  ss << name << ":";
  if (limit<0) {
    limit = nelem;
  } else {
    ss << "(trunc)";
  }

  ss << " [";
  for (size_t i=0; i<limit; i++) {
    if (i % width == 0) ss << "\n";
    ss << " " << delegate::read(base+i);
  }
  ss << " ]"; VLOG(1) << ss.str();
}


GlobalAddress<Graph<Vertex>> E1_index, E2_index, E3_index, E4_index;

void squares( tuple_graph e1, 
              tuple_graph e2,
              tuple_graph e3,
              tuple_graph e4 ) {

  
  on_all_cores( [] { Grappa::Statistics::reset(); } );
    
  // scan tuples and hash join col 1
  VLOG(1) << "Scan tuples, creating index on subject";
  
  double start, end;
  start = Grappa_walltime();


  // TODO: building this graph is not necessary for forward nested loop join (just need untimed filtering tuples phase)
  FullEmpty<GlobalAddress<Graph<Vertex>>> f1;
  privateTask( [&f1,e1] {
    f1.writeXF( Graph<Vertex>::create(e1) );
  });
// TODO: the create() calls should be in parallel but,
// create() uses allreduce, which currently requires a static var :(
// Fix with a symmetric alloc in allreduce
  auto l_E1_index = f1.readFE();
  
  FullEmpty<GlobalAddress<Graph<Vertex>>> f2;
  privateTask( [&f2,e2] {
    f2.writeXF( Graph<Vertex>::create(e2) );
  });
  auto l_E2_index = f2.readFE();
  
  FullEmpty<GlobalAddress<Graph<Vertex>>> f3;
  privateTask( [&f3,e3] {
    f3.writeXF( Graph<Vertex>::create(e3) );
  });
  auto l_E3_index = f3.readFE();
  
  FullEmpty<GlobalAddress<Graph<Vertex>>> f4;
  privateTask( [&f4,e4] {
    f4.writeXF( Graph<Vertex>::create(e4) );
  });
  auto l_E4_index = f4.readFE();

  //auto l_E1_index = f1.readFE();
  //auto l_E2_index = f2.readFE();
  //auto l_E3_index = f3.readFE();
  //auto l_E4_index = f4.readFE();

  // broadcast index addresses to all cores
  on_all_cores([=] {
    E1_index = l_E1_index;
    E2_index = l_E2_index;
    E3_index = l_E3_index;
    E4_index = l_E4_index;
  });


  end = Grappa_walltime();
  
  VLOG(1) << "insertions: " << (e1.nedge+e2.nedge+e3.nedge+e4.nedge)/(end-start) << " per sec";
  index_runtime = end - start;

#if DEBUG
  printAll(E1_index);
#endif

  //
  //TODO: there are plenty of corrections/ optimizations to do here
  // 1. captures by reference in the forall_here bodies
  // 2. moving data that is needed only
  // 3. selection by < degree
  //

  // the forall_localized(forall(...)) here is really just a scan of the E1 edge relation
  forall_localized( E1_index->vs, E1_index->nv, [](int64_t ai, Vertex& a) {
  forall_here_async<&impl::local_gce>( 0, a.nadj, [a,ai](int64_t start, int64_t iters) {
  for ( int64_t i=start; i<start+iters; i++ ) { // forall_here_async serialized for
    ir1_count++; // count(E1)
    auto b_ind = a.local_adj[i];
    auto b_ptr = E2_index->vs + b_ind;
    // lookup b vertex
    remotePrivateTask<&impl::local_gce>(b_ptr.core(), [ai,b_ptr] {
      auto b = *(b_ptr.pointer());
      ir2_count += b.nadj; // count(E1xE2)
      // forall neighbors of b
      forall_here_async<&impl::local_gce>( 0, b.nadj, [ai,b](int64_t start, int64_t iters) {
      for ( int64_t i=start; i<start+iters; i++ ) { // forall_here_async serialized for
        auto c_ind = b.local_adj[i];
        auto c_ptr = E3_index->vs + c_ind;
        // lookup c vertex
        remotePrivateTask<&impl::local_gce>(c_ptr.core(), [ai,b,c_ptr] {
          auto c = *(c_ptr.pointer());
          ir4_count += b.nadj; // count(E1xE2xE3)
          // forall neighbors of c
          forall_here_async<&impl::local_gce>( 0, c.nadj, [ai,b,c](int64_t start, int64_t iters) {
          for ( int64_t i=start; i<start+iters; i++ ) { // forall_here_async serialized for
            auto d_ind = c.local_adj[i];
            auto d_ptr = E4_index->vs + d_ind;
            // lookup d vertex
            remotePrivateTask<&impl::local_gce>(d_ptr.core(), [ai,b,c,d_ptr] {
              auto d = *(d_ptr.pointer());
              // forall neighbors of d
              ir6_count += d.nadj; // count(E1xE2xE3xE4)
              forall_here_async<&impl::local_gce>( 0, d.nadj, [ai,b,c,d](int64_t start, int64_t iters) {
              for ( int64_t i=start; i<start+iters; i++ ) { //forall_here_async serialized 
                auto aprime = d.local_adj[i];
                // check if the a's match
                if ( aprime == ai ) {
                  results_count++;
                }
              }}); // over d->a
            }); // lookup d
          }}); // over c->d
        }); // lookup c
      }}); // over b->c
    }); // lookup b
  }});}); // over a->b
       
        
  end = Grappa_walltime();
  query_runtime = end-start;
  
  Grappa::Statistics::merge_and_print();

  //VLOG(1) << "joins: " << (end-start) << " seconds; total_results_count=" << total_results_count << "; total_ir1_count=" << total_first_join_results << "; total_ir3_count=" << total_second_join_results;
}

void user_main( int * ignore ) {
	
  int64_t nvtx_scale = ((int64_t)1)<<FLAGS_scale;
	int64_t desired_nedge = nvtx_scale * FLAGS_edgefactor;
  
  LOG(INFO) << "scale = " << FLAGS_scale << ", NV = " << nvtx_scale << ", NE = " << desired_nedge;

  // make raw graph edge tuples
  double t;
  tuple_graph tg;
  tg.edges = global_alloc<packed_edge>(desired_nedge);
  LOG(INFO) << "graph generation...";
  t = walltime();
  make_graph( FLAGS_scale, desired_nedge, userseed, userseed, &tg.nedge, &tg.edges );
  double generation_time = walltime() - t;
  LOG(INFO) << "graph_generation: " << generation_time;

  squares( tg, tg, tg, tg );
}


/// Main() entry
int main (int argc, char** argv) {
    Grappa_init( &argc, &argv ); 
    Grappa_activate();

    Grappa_run_user_main( &user_main, (int*)NULL );
    CHECK( Grappa_done() == true ) << "Grappa not done before scheduler exit";
    Grappa_finish( 0 );
}

