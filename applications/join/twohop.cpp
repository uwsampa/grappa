
// util includes
#include "Tuple.hpp"
#include "relation_io.hpp"

// graph500/
#include "../graph500/generator/make_graph.h"
#include "../graph500/generator/utils.h"
#include "../graph500/grappa/timer.h"
#include "../graph500/grappa/common.h"
#include "../graph500/grappa/oned_csr.h" // just for other graph gen stuff besides tuple->csr
#include "../graph500/prng.h"


// Grappa includes
#include <Grappa.hpp>
#include <Cache.hpp>
#include <ParallelLoop.hpp>
#include <GlobalCompletionEvent.hpp>
#include <AsyncDelegate.hpp>
#include <Statistics.hpp>

// Data structure includes
#include "MatchesDHT.hpp"
#include "HashSet.hpp"

#define ASYNCHRONOUS_RESULT 1

bool first_phase = false;
bool second_phase = false;

// file input
DEFINE_string( fin, "", "Input file relation" );
DEFINE_uint64( file_num_tuples, 0, "Number of lines in file" );

DEFINE_bool( print, false, "Print results" );


// outputs
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, join_result_count, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, twohop_count, 0);

GRAPPA_DEFINE_STAT(SimpleStatistic<double>, hash_runtime, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<double>, twohop_runtime, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<double>, count_reduction_runtime, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<double>, read_runtime, 0);


using namespace Grappa;


uint64_t identity_hash( int64_t k ) {
  return k;
}

// local portal to DHT
typedef MatchesDHT<int64_t, Tuple, identity_hash> DHT_type;
DHT_type joinTable;

struct IntPair {
  int64_t x;
  int64_t y;

  bool operator==(const IntPair& o) {
    return o.x==x && o.y==y;
  }
};
std::ostream& operator<< (std::ostream& o, IntPair& i) {
  return o<< "{"<<i.x<<","<<i.y<<"}";
}

// results
uint64_t cat_hash( IntPair k ) {
  return ((0xffffffff&k.x) << 32) | (0xffffffff&k.y);
}
#if ASYNCHRONOUS_RESULT
typedef HashSet<IntPair, cat_hash> Results_type;
#else 
typedef MatchesDHT<IntPair, bool, cat_hash> Results_type; 
#endif
Results_type results;


void scanAndHash( GlobalAddress<Tuple> tuples, size_t num ) {
  forall_localized( tuples, num, [](int64_t i, Tuple& t) {
    int64_t key = t.columns[0];

    VLOG(2) << "insert " << key << " | " << t;
    joinTable.insert( key, t );
  });
}


double read_start, read_end;

void twohop( GlobalAddress<Tuple> tuples, size_t num_tuples ) {

  on_all_cores( [] { Grappa::Statistics::reset(); } );
  read_runtime = read_end-read_start;
  
  
  // scan tuples and hash join col
  VLOG(1) << "Scan tuples, creating index on subject";
  
  
  double start, end;
  start = Grappa_walltime();
  {
    scanAndHash( tuples, num_tuples );
  } 
  end = Grappa_walltime();
  
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


  //on_all_cores([]{Grappa_start_profiling();});
  start = Grappa_walltime();
  VLOG(1) << "Starting 1st join";
  forall_localized( tuples, num_tuples, [](int64_t i, Tuple& t) {
    int64_t key = t.columns[1];
   
    // will pass on this first vertex to compare in the select 
    int64_t x1 = t.columns[0];

#if SORTED_KEYS
    // first join
    uint64_t results_idx;
    size_t num_results = joinTable.lookup( key, &results_idx );
    
    DVLOG(4) << "key " << t << " finds (" << results_idx << ", " << num_results << ")";
   
    // iterate over the first join results in parallel
    // (iterations must spawn with synch object `local_gce`)
    forall_here_async_public( results_idx, num_results, [x1](int64_t start, int64_t iters) {
      Tuple subset_stor[iters];
      Incoherent<Tuple>::RO subset( IndexBase+start, iters, &subset_stor );
#else // MATCHES_DHT
    // first join
    GlobalAddress<Tuple> results_addr;
    size_t num_results = joinTable.lookup( key, &results_addr );
    
    // iterate over the first join results in parallel
    // (iterations must spawn with synch object `local_gce`)
    /* not yet supported: forall_here_async_public< GCE=&local_gce >( 0, num_results, [x1,results_addr](int64_t start, int64_t iters) { */
    forall_here_async( 0, num_results, [x1,results_addr](int64_t start, int64_t iters) { 
      Tuple subset_stor[iters];
      Incoherent<Tuple>::RO subset( results_addr+start, iters, subset_stor );
#endif
 
  /*//phased profiling    
     if (Grappa::mycore()==0) {
     if (join_result_count > 1000000 && !first_phase) {
      first_phase = true;
      call_on_all_cores([]{Grappa_stop_profiling();}); 
      call_on_all_cores([]{Grappa_start_profiling();}); 
      }
  
     if (join_result_count > 22000000 && !second_phase) { 
      second_phase = true;
      call_on_all_cores([]{Grappa_stop_profiling();}); 
      }
      }
      */
      
      

      join_result_count += iters; 

#if ASYNCHRONOUS_RESULT
      // allocate space for asynchronous insertions
      char pool_storage[results.insertion_pool_size( iters )];
      MessagePool pool( pool_storage, sizeof(pool_storage) );
#endif

      for ( int64_t i=0; i<iters; i++ ) {

        int64_t x3 = subset[i].columns[1];
        IntPair r = {x1,x3}; 
#if ASYNCHRONOUS_RESULT
        results.insert_async( r, pool );
#else
        //if ( !results.insert_unique( r ) ) { // hard-to-predict branch? Could change to conditional increment
          twohop_count += 1;
          if ( FLAGS_print ) {
            VLOG(1) << x1 << " " << x3;
          }
       // }
#endif
      } 
    }); // end loop over join results
  }); // end loop over relation

  end = Grappa_walltime();
  twohop_runtime = end - start;

#if ASYNCHRONOUS_RESULT
    twohop_count = results.size();
    count_reduction_runtime = Grappa_walltime() - end;
#endif 

//  on_all_cores([]{Grappa_stop_profiling();});
  Grappa::Statistics::merge_and_print();
}

void user_main( int * ignore ) {

  GlobalAddress<Tuple> tuples;
  size_t num_tuples;
 

  if ( FLAGS_fin == "" ) {
    VLOG(1) << "Generating some data";
    //tuples = generate_data( FLAGS_scale, FLAGS_edgefactor, &num_tuples );
    //
    //TODO
    exit(1);
  } else {
    VLOG(1) << "Reading data from " << FLAGS_fin;
    
    tuples = Grappa_typed_malloc<Tuple>( FLAGS_file_num_tuples );
    read_start = Grappa_walltime();
    readTuples( FLAGS_fin, tuples, FLAGS_file_num_tuples );
    read_end = Grappa_walltime();
    num_tuples = FLAGS_file_num_tuples;
    
    //print_array( "file tuples", tuples, FLAGS_file_num_tuples, 1, 200 );
  }

  VLOG(1) << "initializing join table";
  DHT_type::init_global_DHT( &joinTable, num_tuples );
  VLOG(1) << "initializing results table";
  Results_type::init_global_DHT( &results, num_tuples*25 );

  twohop( tuples, num_tuples ); 
}


/// Main() entry
int main (int argc, char** argv) {
    Grappa_init( &argc, &argv ); 
    Grappa_activate();

    Grappa_run_user_main( &user_main, (int*)NULL );
    CHECK( Grappa_done() == true ) << "Grappa not done before scheduler exit";
    Grappa_finish( 0 );
}


