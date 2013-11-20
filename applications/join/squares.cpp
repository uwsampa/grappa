/*
 * Example of:
 * (partial) triangle listing
 *
 * A(x,z) :- R(x,y), R(y,z), R(z,x)
 */


// util includes
#include "Tuple.hpp"
#include "relation_IO.hpp"

// graph500/
#include "../graph500/generator/make_graph.h"
#include "../graph500/generator/utils.h"
#include "../graph500/grappa/timer.h"
#include "../graph500/grappa/common.h"
#include "../graph500/grappa/oned_csr.h" // just for other graph gen stuff besides tuple->csr
#include "../graph500/prng.h"


// Grappa includes
#include <Grappa.hpp>
#include "MatchesDHT.hpp"
#include <Cache.hpp>
#include <ParallelLoop.hpp>
#include <GlobalCompletionEvent.hpp>
#include <AsyncDelegate.hpp>
#include <Statistics.hpp>

// command line parameters

// file input
DEFINE_string( fin, "", "Input file relation" );
DEFINE_uint64( file_num_tuples, 0, "Number of lines in file" );

// generating input data
DEFINE_uint64( scale, 7, "Log of number of vertices" );
DEFINE_uint64( edgefactor, 16, "Median degree to try to generate" );
DEFINE_bool( undirected, false, "Generated graph implies undirected edges" );

DEFINE_bool( print, false, "Print results" );

GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, edges_transfered, 0);

// outputs
GRAPPA_DEFINE_STAT(SimpleStatistic<double>, query_runtime, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, ir1_count, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, ir2_count, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, ir3_count, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, ir4_count, 0);
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


void squares( tuple_graph& e1, 
              tuple_graph& e2,
              tuple_graph& e3,
              tuple_graph& e4 ) {

  
  on_all_cores( [] { Grappa::Statistics::reset(); } );
    
  // scan tuples and hash join col 1
  VLOG(1) << "Scan tuples, creating index on subject";

  GlobalAddress<Graph> E1_index, E2_index, E3_index, E4_index;
  
  double start, end;
  start = Grappa_walltime();

  // TODO: building this graph is not necessary for forward nested loop join (just need untimed filtering tuples phase)
  FullEmpty<GlobalAddress<Graph>> f1();
  privateTask( [&f1,e1] {
//    forall_localized( tuples, num_tuples, [](int64_t i, Tuple& t) {
//      int64_t key = t.columns[0];
//      E1_table.insert( key, t );
//    });
    f1.writeXF( Graph<Vertex>::create(e1) );
  }
  
  FullEmpty<int> f2();
  privateTask( [&f2,e2] {
//    forall_localized( tuples, num_tuples, [](int64_t i, Tuple& t) {
//      int64_t key = t.columns[0];
//      E2_table.insert( key, t );
//    });
    f2.writeXF( Graph<Vertex>::create(e2) );
  }
  
  FullEmpty<int> f3();
  privateTask( [&f3,e3] {
//    forall_localized( tuples, num_tuples, [](int64_t i, Tuple& t) {
//      int64_t key = t.columns[0];
//      E3_table.insert( key, t );
//    });
    f3.writeXF( Graph<Vertex>::create(e3) );
  }
  
  FullEmpty<int> f4();
  privateTask( [&f4,e4] {
//    forall_localized( tuples, num_tuples, [](int64_t i, Tuple& t) {
//      int64_t key = t.columns[0];
//      E4_table.insert( key, t );
//    });
    f4.writeXF( Graph<Vertex>::create(e4) );
  }
  E1_index = f1.readFE();
  E2_index = f2.readFE();
  E3_index = f3.readFE();
  E4_index = f4.readFE();

  end = Grappa_walltime();
  
  VLOG(1) << "insertions: " << (e1.nedges+e2.nedges+e3.nedges+e4.nedges)/(end-start) << " per sec";
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
  forall_here_async( 0, a.nadj, [a,ai](int64_t start, int64_t iters) {
  for ( int64_t i=start; i<start+iters; i++ ) { // forall_here_async serialized for
    auto b_ind = a.local_adj[i];
    auto b_ptr = E2_index->vs + b_ind;
    // lookup b vertex
    delegate::call_async(b_ptr.core(), [ai,b_ptr] {
      auto b = *(b_ptr.pointer());
      forall_here_async( 0, b.nadj, [ai,b](int64_t start, int64_t iters) {
      for ( int64_t i=start; i<start+iters; i++ ) { // forall_here_async serialized for
        auto c_ind = b.local_adj[i];
        auto c_ptr = E3_index->vs + c_ind;
        // lookup c vertex
        delegate::call_async(c_ptr.core(), [ai,b,c_ptr] {
          auto c = *(c_ptr.pointer());
          forall_here_async( 0, c.nadj, [ai,b,c](int64_t start, int64_t iters) {
          for ( int64_t i=start; i<start+iters; i++ ) { // forall_here_async serialized for
            auto d_ind = c.local_adj[i];
            auto d_ptr = E4_index->vs + d_ind;
            // lookup d vertex
            delegate::call_async(d_ptr.core(), [ai,b,c,d_ptr] {
              auto d = *(d_ptr.pointer());
              forall_here_async( 0, d.nadj, [a,b,c,d](int64_t start, int64_t iters) {
              for ( int64_t i=start; i<start+iters; i++ ) { //forall_here_async serialized 
                auto aprime = d.local_adj[i];
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

  // make raw graph edge tuples
  double t;
  tuple_graph tg;
  tg.edges = global_alloc<packed_edge>(desired_nedge);
  LOG(INFO) << "graph generation...";
  t = walltime();
  make_graph( FLAGS_scale, desired_nedge, userseed, userseed, &tg.nedge, &tg.edges );
  generation_time = walltime() - t;
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

