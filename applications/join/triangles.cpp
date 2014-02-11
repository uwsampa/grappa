/*
 * Example of:
 * (partial) triangle listing
 *
 * A(x,z) :- R(x,y), R(y,z), R(z,x)
 */


// util includes
#include "Tuple.hpp"
#include "relation_io.hpp"

// graph500/
#include "generator/make_graph.h"
#include "generator/utils.h"
#include "grappa/timer.h"
#include "grappa/common.h"
#include "grappa/oned_csr.h" // just for other graph gen stuff besides tuple->csr
#include "prng.h"


// Grappa includes
#include <Grappa.hpp>
#include "MatchesDHT.hpp"
#include <Cache.hpp>
#include <ParallelLoop.hpp>
#include <GlobalCompletionEvent.hpp>
#include <AsyncDelegate.hpp>
#include <Metrics.hpp>

// command line parameters

// file input
DEFINE_string( fin, "", "Input file relation" );
DEFINE_uint64( file_num_tuples, 0, "Number of lines in file" );

// generating input data
DEFINE_uint64( scale, 7, "Log of number of vertices" );
DEFINE_uint64( edgefactor, 16, "Median degree to try to generate" );
DEFINE_bool( undirected, false, "Generated graph implies undirected edges" );

DEFINE_bool( print, false, "Print results" );


// outputs
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, triangles_runtime, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, first_join_count, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, first_join_select_count, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, second_join_count, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, second_join_select_count, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, triangle_count, 0);

GRAPPA_DEFINE_METRIC(SimpleMetric<double>, hash_runtime, 0);

GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, edges_transfered, 0);


using namespace Grappa;

std::ostream& operator<<( std::ostream& o, const Tuple& t ) {
  o << "T( ";
  for (uint64_t i=0; i<TUPLE_LEN; i++) {
    o << t.columns[i] << ", ";
  }
  o << ")";
}

typedef uint64_t Column;

uint64_t identity_hash( int64_t k ) {
  return k;
}

// local portal to DHT
typedef MatchesDHT<int64_t, Tuple, identity_hash> DHT_type;
DHT_type joinTable;

// local RO copies
GlobalAddress<Tuple> local_tuples;
Column local_join1Left, local_join1Right, local_join2Right;
Column local_select;
GlobalAddress<Tuple> IndexBase;

// local counters
uint64_t local_triangle_count;
uint64_t local_first_join_results;
uint64_t local_second_join_results;


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


GlobalAddress<Tuple> generate_data( size_t scale, size_t edgefactor, size_t * num ) {
  // initialize rng 
  //init_random(); 
  //userseed = 10;
  uint64_t N = (1L<<scale);
  uint64_t desired_nedges = edgefactor*N;

  VLOG(1) << "Generating edges (desired " << desired_nedges << ")";
  tuple_graph tg;
  make_graph( scale, desired_nedges, userseed, userseed, &tg.nedge, &tg.edges );
  VLOG(1) << "edge count: " << tg.nedge;

  size_t nedge = FLAGS_undirected ? 2*tg.nedge : tg.nedge;

  // allocate the tuples
  GlobalAddress<Tuple> base = Grappa::global_alloc<Tuple>( nedge );

  // copy and transform from edge representation to Tuple representation
  forall(tg.edges, tg.nedge, [base](int64_t start, int64_t n, packed_edge * first) {
    // FIXME: I know write_async messages are one globaladdress 
    // and one tuple, but make it encapsulated
    int64_t num_messages =  FLAGS_undirected ? 2*n : n;

    for (int64_t i=0; i<n; i++) {
      auto e = first[i];

      Tuple t;
      t.columns[0] = get_v0_from_edge( &e );
      t.columns[1] = get_v1_from_edge( &e );

      if ( FLAGS_undirected ) {
        delegate::write<async>(base+start+2*i, t ); 
        t.columns[0] = get_v1_from_edge( &e );
        t.columns[1] = get_v0_from_edge( &e );
        delegate::write<async>(base+start+2*i+1, t ); 
      } else {
        delegate::write<async>(base+start+i, t ); 
      }
      // optimally I'd like async WO cache op since this will coalesce the write as well
     }
   });

  *num = nedge;

  //print_array( "generated tuples", base, nedge, 1 );

  // TODO: remove self-edges and duplicates (e.g. prefix sum and compaction)

  return base;
}

void scanAndHash( GlobalAddress<Tuple> tuples, size_t num ) {
  forall( tuples, num, [](int64_t i, Tuple& t) {
    int64_t key = t.columns[local_join1Left];

    VLOG(2) << "insert " << key << " | " << t;
    joinTable.insert( key, t );
  });
}


void triangles( GlobalAddress<Tuple> tuples, size_t num_tuples, Column ji1, Column ji2, Column ji3, Column s ) {
  // initialization
  on_all_cores( [tuples, ji1, ji2, ji3, s] {
    local_tuples = tuples;
    local_join1Left = ji1;
    local_join1Right = ji2;
    local_join2Right = ji3;
    local_select = s;
    local_triangle_count = 0;
    local_first_join_results = 0;
    local_second_join_results = 0;
  });
  
  on_all_cores( [] { Grappa::Metrics::reset(); } );
    
  // scan tuples and hash join col 1
  VLOG(1) << "Scan tuples, creating index on subject";
  
  double start, end;
  start = Grappa::walltime();
  {
    scanAndHash( tuples, num_tuples );
  } 
  end = Grappa::walltime();
  
  VLOG(1) << "insertions: " << num_tuples/(end-start) << " per sec";
  hash_runtime = end - start;

#if DEBUG
  printAll(tuples, num_tuples);
#endif

  // tell the DHT we are done with inserts
  VLOG(1) << "DHT setting to RO";
#if SORTED_KEYS
  GlobalAddress<Tuple> index_base = DHT_type::set_RO_global( &joinTable );
  on_all_cores([index_base] {
      IndexBase = index_base;
  });
#else
  DHT_type::set_RO_global( &joinTable );
#endif

  start = Grappa::walltime();
  VLOG(1) << "Starting 1st join";
  forall( tuples, num_tuples, [](int64_t i, Tuple& t) {
    int64_t key = t.columns[local_join1Right];
   
    // will pass on this first vertex to compare in the select 
    int64_t x1 = t.columns[local_join1Left];

#if SORTED_KEYS
    // first join
    uint64_t results_idx;
    size_t num_results = joinTable.lookup( key, &results_idx );
    
    DVLOG(4) << "key " << t << " finds (" << results_idx << ", " << num_results << ")";
   
    // iterate over the first join results in parallel
    // (iterations must spawn with synch object `local_gce`)
    edges_transfered += num_results;
    forall_here<unbound,async>(results_idx, num_results, [x1](int64_t start, int64_t iters) {
      Tuple subset_stor[iters];
      Incoherent<Tuple>::RO subset( IndexBase+start, iters, &subset_stor );
#else // MATCHES_DHT
    // first join
    GlobalAddress<Tuple> results_addr;
    size_t num_results = joinTable.lookup( key, &results_addr );
    
    // iterate over the first join results in parallel
    // (iterations must spawn with synch object `local_gce`)
    /* not yet supported: forall_here<unbound,async, GCE=&local_gce >( 0, num_results, [x1,results_addr](int64_t start, int64_t iters) { */
    edges_transfered += num_results;
    forall_here<async>( 0, num_results, [x1,results_addr](int64_t start, int64_t iters) { 
      Tuple subset_stor[iters];
      Incoherent<Tuple>::RO subset( results_addr+start, iters, subset_stor );
#endif

      //local_first_join_results+=iters;
      first_join_count+=iters;
      for ( int64_t i=0; i<iters; i++ ) {
        int64_t key = subset[i].columns[local_join2Right];

        int64_t x2 = subset[i].columns[0];
        VLOG(5) << "COMPARING x1=" << x1 << " < x2=" << x2; 
        
        if ( x1 < x2 ) {  // early select on ordering
          first_join_select_count+=1; // count after select

#if SORTED_KEYS
          // second join
          uint64_t results_idx;
          size_t num_results = joinTable.lookup( key, &results_idx );

          VLOG(5) << "results key " << key << " (n=" << num_results;
          
          // iterate over the second join results in parallel
          // (iterations must spawn with synch object `local_gce`)
          edges_transfered += num_results;
          forall_here<unbound,async>(results_idx, num_results, [x1](int64_t start, int64_t iters) {
            Tuple subset_stor[iters];
            Incoherent<Tuple>::RO subset( IndexBase+start, iters, subset_stor );
#else // MATCHES_DHT
          // second join
          GlobalAddress<Tuple> results_addr;
          size_t num_results = joinTable.lookup( key, &results_addr );

          VLOG(5) << "results key " << key << " (n=" << num_results;
          
          // iterate over the second join results in parallel
          // (iterations must spawn with synch object `local_gce`)
          /* not yet supported: forall_here<unbound,async, GCE=&local_gce >( 0, num_results, [x1,results_addr](int64_t start, int64_t iters) { */
          edges_transfered += num_results;
          forall_here<async>( 0, num_results, [x1,x2,results_addr](int64_t start, int64_t iters) {
            Tuple subset_stor[iters];
            Incoherent<Tuple>::RO subset( results_addr+start, iters, subset_stor );
#endif
            second_join_count += iters;

            for ( int64_t i=0; i<iters; i++ ) {
              int64_t r = subset[i].columns[0];
              if ( x2 < r ) {  // select on ordering 
                second_join_select_count+=1;
                if ( subset[i].columns[local_select] == x1 ) { // select on triangle
                  if (FLAGS_print) {
                    VLOG(1) << x1 << " " << x2 << " " << r;
                  }
                  triangle_count++;
                }
              } // end select 2
            } // (end loop body for over 2nd join results)
          }); // end loop over 2nd join results
        } // end select 1
      } // (end loop body for over 1st join results)
    }); // end loop over 1st join results
  }); // end outer loop over tuples
         
  
  end = Grappa::walltime();
  triangles_runtime = end-start;
  
  //uint64_t total_triangle_count = Grappa::reduce<uint64_t, collective_add>( &local_triangle_count ); 
  //uint64_t total_first_join_results = Grappa::reduce<uint64_t, collective_add>( &local_first_join_results ); 
  //uint64_t total_second_join_results = Grappa::reduce<uint64_t, collective_add>( &local_second_join_results ); 
 
  //triangle_count = total_triangle_count;
  //first_join_count = total_first_join_results;
  //second_join_count = total_second_join_results;

  Grappa::Metrics::merge_and_print();

  //VLOG(1) << "joins: " << (end-start) << " seconds; total_triangle_count=" << total_triangle_count << "; total_first_join_count=" << total_first_join_results << "; total_second_join_count=" << total_second_join_results;
}

int main(int argc, char* argv[]) {
  Grappa::init(&argc, &argv);
  Grappa::run([]{

    GlobalAddress<Tuple> tuples;
    size_t num_tuples;

    if ( FLAGS_fin == "" ) {
      VLOG(1) << "Generating some data";
      tuples = generate_data( FLAGS_scale, FLAGS_edgefactor, &num_tuples );
    } else {
      VLOG(1) << "Reading data from " << FLAGS_fin;
    
    tuples = Grappa::global_alloc<Tuple>( FLAGS_file_num_tuples );
    readEdges( FLAGS_fin, tuples, FLAGS_file_num_tuples );
    num_tuples = FLAGS_file_num_tuples;
    
      print_array( "file tuples", tuples, FLAGS_file_num_tuples, 1, 200 );
    }

    DHT_type::init_global_DHT( &joinTable, 64 );

    Column joinIndex1 = 0; // subject
    Column joinIndex2 = 1; // object
    Column select = 1; // object

    // triangle (assume one index to build)
    triangles( tuples, num_tuples, joinIndex1, joinIndex2, joinIndex2, select ); 
  });
  Grappa::finalize();
}
