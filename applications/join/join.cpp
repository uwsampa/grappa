/*
 * Example of:
 *
 * A(z,t) :- R(x,y), R(y,z), R(z,t)
 */


// util includes
#include "Tuple.hpp"
#include "relation_IO.hpp"


// Grappa includes
#include <Grappa.hpp>
#include "MatchesDHT.hpp"
#include <Cache.hpp>
#include <AsyncParallelFor.hpp>

// command line parameters
DEFINE_uint64( numTuples, 32, "Number of tuples to generate" );
DEFINE_string( in, "", "Input file relation" );
DEFINE_bool( print, false, "Print results" );



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
Column local_joinIndex1, local_joinIndex2, local_joinIndex3;


// TODO: incorporate the edge tuples generation (although only does triples)
void tuple_gen( Tuple * slot ) {
  Tuple r;
  for ( uint64_t i=0; i<TUPLE_LEN; i++ ) {
    r.columns[i] = rand()%FLAGS_numTuples; 
  }
  *slot = r;
}

void scanAndHash( Tuple * t ) {
  int64_t key = t->columns[local_joinIndex1];
  Tuple val = *t;

  VLOG(2) << "insert " << key << " | " << val;
  joinTable.insert( key, val );
}

void secondJoin( GlobalAddress<Tuple> start, int64_t iters);

void firstJoin( Tuple * t ) {
  int64_t key = t->columns[local_joinIndex2];

  GlobalAddress<Tuple> results;
  size_t num_results = joinTable.lookup( key, &results );
  DVLOG(4) << "key " << *t << " finds (" << results << ", " << num_results << ")";

  // inner for loop
  async_parallel_for<Tuple, secondJoin, joinerSpawn<Tuple,secondJoin,ASYNC_PAR_FOR_DEFAULT>, ASYNC_PAR_FOR_DEFAULT >( results, num_results );

}


void printAll( GlobalAddress<Tuple> ts, size_t num ) {
  Tuple storage[num];
  Incoherent<Tuple>::RO c_ts( ts, num, storage );
  for (uint64_t i=0; i<num; i++) {
    VLOG(1) << c_ts[i];
  }
}


void secondJoin( GlobalAddress<Tuple> start, int64_t iters ) {
  Tuple tuples_s[iters];
  Incoherent<Tuple>::RO tuples(start, iters, tuples_s);
  for (int64_t i = 0; i < iters; i++) {

    int64_t key = tuples[i].columns[local_joinIndex3];

    GlobalAddress<Tuple> results;
    size_t num_results = joinTable.lookup( key, &results );

    if (FLAGS_print) {
      VLOG(1) << "results key " << key << " (n=" << num_results;
      printAll( results, num_results );
    }
  }
}
  

LOOP_FUNCTOR(join_f_init2, nid, ((GlobalAddress<Tuple>,tuples)) ((Column,ji1)) ((Column,ji2)) ((Column,ji3)) ) {
  local_tuples = tuples;
  local_joinIndex1 = ji1;
  local_joinIndex2 = ji2;
  local_joinIndex3 = ji3;
}

LOOP_FUNCTION(global_joiner_reset, nid) {
  global_joiner.reset();
}

LOOP_FUNCTION(global_joiner_wait, nid) {
  global_joiner.wait();
}



void join2( GlobalAddress<Tuple> tuples, Column ji1, Column ji2, Column ji3 ) {
  // initialization
  on_all_nodes( join_f_init2, tuples, ji1, ji2, ji3 );
  
  // scan tuples and hash join col 1
  VLOG(1) << "Scan tuples, creating index on subject";
  double start = Grappa_walltime();
  forall_local<Tuple, scanAndHash>( tuples, FLAGS_numTuples );
  double end = Grappa_walltime();
  VLOG(1) << "insertions: " << (end-start)/FLAGS_numTuples << " per sec";

#if DEBUG
  printAll(tuples, FLAGS_numTuples);
#endif

  // tell the DHT we are done with inserts
  VLOG(1) << "DHT setting to RO";
  DHT_type::set_RO_global( &joinTable );

  // firstJoin contains unsynced nested parallel loops, so require
  // this surrounding join
  
  // FIXME: this synchronization is overly complicated
  start = Grappa_walltime();
  { global_joiner_reset f; fork_join_custom(&f); }
  VLOG(1) << "Starting 1st join";
  forall_local<Tuple, firstJoin>( tuples, FLAGS_numTuples );
  { global_joiner_wait f; fork_join_custom(&f); }
  end = Grappa_walltime();
  VLOG(1) << "joins: " << (end-start) << " seconds";
}

void user_main( int * ignore ) {

  GlobalAddress<Tuple> tuples = Grappa_typed_malloc<Tuple>( FLAGS_numTuples );

  if ( FLAGS_in == "" ) {
    VLOG(1) << "Generating some data";
    forall_local<Tuple, tuple_gen>( tuples, FLAGS_numTuples );
  } else {
    VLOG(1) << "Reading data from " << FLAGS_in;
    readTuples( FLAGS_in, tuples, FLAGS_numTuples );
  }

  DHT_type::init_global_DHT( &joinTable, 64 );

  Column joinIndex1 = 0; // subject
  Column joinIndex2 = 1; // object

  // double join case (assuming one index to build)
  join2( tuples, joinIndex1, joinIndex2, joinIndex2 ); 

}


/// Main() entry
int main (int argc, char** argv) {
    Grappa_init( &argc, &argv ); 
    Grappa_activate();

    Grappa_run_user_main( &user_main, (int*)NULL );
    CHECK( Grappa_done() == true ) << "Grappa not done before scheduler exit";
    Grappa_finish( 0 );
}



// insert conflicts use java-style arraylist, enabling memcpy for next step of join
