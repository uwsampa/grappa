#include <Grappa.hpp>
#include "MatchesDHT.hpp"
#include <Cache.hpp>
#include <AsyncParallelFor.hpp>

#include <string>
#include <fstream>
#include <vector>

DEFINE_uint64( numTuples, 32, "Number of tuples to generate" );


std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
  std::stringstream ss(s);
  std::string item;
  while(std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  return split(s, delim, elems);
}

#define TUPLE_LEN 3
struct Tuple {
  int64_t columns[TUPLE_LEN];
};

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
    r.columns[i] = rand()%10; 
  }
  *slot = r;
}

void scanAndHash( Tuple * t ) {
  int64_t key = t->columns[local_joinIndex1];
  Tuple val = *t;

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

    VLOG(1) << "results key " << key << " (n=" << num_results;
    printAll( results, num_results );
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
  forall_local<Tuple, scanAndHash>( tuples, FLAGS_numTuples );

  printAll(tuples, FLAGS_numTuples);

  // tell the DHT we are done with inserts
  VLOG(1) << "DHT setting to RO";
  DHT_type::set_RO_global( &joinTable );

  // firstJoin contains unsynced nested parallel loops, so require
  // this surrounding join
  
  // FIXME: this synchronization is overly complicated
  { global_joiner_reset f; fork_join_custom(&f); }
  VLOG(1) << "Starting 1st join";
  forall_local<Tuple, firstJoin>( tuples, FLAGS_numTuples );
  { global_joiner_wait f; fork_join_custom(&f); }
}

void user_main( int * ignore ) {

  GlobalAddress<Tuple> tuples = Grappa_typed_malloc<Tuple>( FLAGS_numTuples );

  VLOG(1) << "Generating some data";
  //forall_local<Tuple, tuple_gen>( tuples, FLAGS_numTuples );
  std::ifstream testfile("testcase.txt");
  std::string line;
  int fin = 0;
  if (testfile.is_open()) {
    while (testfile.good() && (fin++)<FLAGS_numTuples) {
      std::getline( testfile, line );
      std::cout<< "L " << line << std::endl;
      Incoherent<Tuple>::WO lr(tuples, 1);
      std::vector<std::string> tokens = split( line, ' ' );
     std::cout<< tokens[0] << " " << tokens[1] << std::endl;
      (*lr).columns[0] = std::stoi(tokens[0]);
      (*lr).columns[1] = 0; 
      (*lr).columns[2] = stoi(tokens[1]);
    }
    testfile.close();
  }

  
  Incoherent<Tuple>::WO r(tuples, 6);
  

  DHT_type::init_global_DHT( &joinTable, 64 );

  Column joinIndex1 = 0; // subject
  Column joinIndex2 = 2; // object
  Column joinIndex3 = 2; // object

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
