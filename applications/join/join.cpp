#include "DHT.hpp"

DEFINE_uint64( numTuples, 32, "Number of tuples to generate" );

#define TUPLE_LEN 3
struct Tuple {
  int64_t columns[TUPLE_LEN];
};

typedef uint64_t Column;

uint64_t identity_hash( int64_t k ) {
  return k;
}

// local portal to DHT
DHT<int64_t, Tuple, identity_hash> joinTable;

// local RO copies
GlobalAddress<Tuple> local_tuples;
Column local_joinIndex1, local_joinIndex2;


// TODO: incorporate the edge tuples generation (although only does triples)
void tuple_gen( Tuple * slot ) {
  Tuple r;
  for ( uint64_t i=0; i<TUPLE_LEN; i++ ) {
    r.columns[i] = rand()*10; 
  }
  *slot = r;
}

void scanAndHash( Tuple * t ) {
  int64_t key = t->columns[local_joinIndex1];
  Tuple val = *t;

  joinTable.insert( key, val );
}

void scanAndLookup( Tuple * t ) {
  int64_t key = t->columns[local_joinIndex2];

  Tuple result; // TODO multiple returns, or indices
  joinTable.lookup( key, &result );

  /* TODO operation on result */
}

void secondJoin( GlobalAddress<Tuple> t );

void firstJoin( Tuple * t ) {
  int64_t key = t->columns[local_joinIndex2];

  uint64_t maxResults = 10;
  GlobalAddress<Tuple> results = Grappa_typed_malloc( maxResults ); // TODO multiple returns, or indices
  joinTable.lookup( key, results, maxResults );
// get all results...

  DynamicJoiner j;
  j.resetAll();

  // inner for loop
  async_parallel_for<secondJoin>( results, 0, numResults );
  // TODO just return a pointer
  
  j.wait();
  Grappa_free( results );

  /*
   * We need a clean way to provide results array until it is done.
   *
   * Most paradigmatic would be make this look as inline so that
   * deallocation is after joining on the for loop.
   *
   * Or an async_parallel_for finish callback where we put
   * a continuation that deallocates the memory.
   */
}

void secondJoin( GlobalAddress<Tuple> t ) {
  int64_t key = t->columns[local_joinIndex3];

  uint64_t maxResults = 10;
  GlobalAddress<Tuple> results = Grappa_typed_malloc( maxResults ); // TODO multiple returns, or indices
  joinTable.lookup( key, results, maxResults );
// get all results...

  printAll( results );
}
  


LOOP_FUNCTOR(join_f_init, nid, ((GlobalAddress<Tuple>,tuples)) ((Column,ji1)) ((Column,ji2)) ) {
  local_tuples = tuples;
  local_joinIndex1 = ji1;
  local_joinIndex2 = ji2;
}

LOOP_FUNCTOR(join_f_init2, nid, ((GlobalAddress<Tuple>,tuples)) ((Column,ji1)) ((Column,ji2)) ((Column,ji2)) ) {
  local_tuples = tuples;
  local_joinIndex1 = ji1;
  local_joinIndex2 = ji2;
  local_joinIndex3 = ji3;
}

void join( GlobalAddress<Tuple> tuples, Column ji1, Column ji2 ) {
  // initialization
  on_all_nodes( join_f, tuples, ji1, ji2 );
 
  // scan tuples and hash join col 1
  forall_local<Tuple, scanAndHash>( tuples, FLAGS_numTuples );

  // scan tuples looking up join col 2
  forall_local<Tuple, scanAndLookup>( tuples, FLAGS_numTuples );
}

void join2( GlobalAddress<Tuple> tuples, Column ji1, Column ji2, Column ji3 ) {
  // initialization
  on_all_nodes( join_f_init2, tuples, ji1, ji2, ji3 );
  
  // scan tuples and hash join col 1
  forall_local<Tuple, scanAndHash>( tuples, FLAGS_numTuples );

  forall_local<Tuple, firstJoin>( tuples, FLAGS_numTuples );

}

void user_main( int * ignore ) {

  GlobalAddress<Tuple> tuples = Grappa_typed_malloc<Tuple>( FLAGS_numTuples );

  forall_local<Tuple, tuple_gen>( tuples, FLAGS_numTuples );

  DHT<int64_t, Tuple, identity_hash>::init_global_DHT( &joinTable, 64 );

  Column joinIndex1 = 0;
  Column joinIndex2 = 2;

  join( tuples, joinIndex1, joinIndex2 );


  // double join case (assuming one index to build)
  join2( tuples, joinIndex1, joinIndex2, joinIndex3 ); 
  

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
