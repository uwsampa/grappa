
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "npb_intsort.hpp"
#include "randlc.hpp"
#include <Grappa.hpp>
#include <Communicator.hpp>
#include <Addressing.hpp>
#include <GlobalAllocator.hpp>
#include <Collective.hpp>
#include <ParallelLoop.hpp>
#include <unistd.h>
#include <getopt.h>

using namespace Grappa;

////////////
// Globals
////////////

static const int MIN_PROCS = 1;
static const int MAX_PROCS = 1024;

static const int MAX_ITERATIONS = 10;

typedef int key_t;

// global const for one sort
int scale;
int log2buckets;
int log2maxkey;
int64_t nkeys;
int nbuckets;
key_t maxkey;

double my_seed;

NPBClass npbclass;

GlobalAddress<key_t> key_array;

void init_seed() {
  my_seed = find_my_seed(  Grappa::mycore(), 
                           Grappa::cores(), 
                           4*(long)nkeys,
                           314159265.00,      // Random number gen seed
                           1220703125.00 );   // Random number gen mult
}

inline int next_seq_element() {
    const double a = 1220703125.00; // Random number gen mult
    int k = maxkey/4;

    double x = randlc(&my_seed, &a);
          x += randlc(&my_seed, &a);
          x += randlc(&my_seed, &a);
          x += randlc(&my_seed, &a);
    
    return k*x;
}

void rank(int iteration) {
  // do nothing
}

void user_main(void * ignore) {
  int             i, iteration, itemp;
  double          total_time, generation_time;
  
  //  Printout initial NPB info 
  printf( "NAS Parallel Benchmarks 3.3 -- IS Benchmark in Grappa\n");
  printf( "nkeys:       %ld  (class %c)\n", (long)nkeys, npb_class_char(npbclass));
  printf( "niterations: %d\n", MAX_ITERATIONS);
  printf( "cores:       %d\n", cores());

  // Check to see whether total number of processes is within bounds.
  if( cores() < MIN_PROCS || cores() > MAX_PROCS) {
      printf( "\n ERROR: number of processes %d not within range %d-%d"
             "\n Exiting program!\n\n", cores(), MIN_PROCS, MAX_PROCS);
      return;
  }

  auto _key_array = Grappa_typed_malloc<key_t>(nkeys);

  generation_time = Grappa_walltime();

  // initialize all cores
  call_on_all_cores([_key_array]{
    init_seed();
    key_array = _key_array;
  });

  // Generate random number sequence and subsequent keys on all procs
  // Note: the distribution should be roughly Gaussian
  forall_localized(key_array, nkeys, [](int64_t i, key_t& key){
    key = next_seq_element();
  });
  
  generation_time = Grappa_walltime() - generation_time;
  std::cerr << "generation_time: " << generation_time << "\n";

  // Do one interation for free (i.e., untimed) to guarantee initialization of  
  // all data and code pages and respective tables
  rank( 1 );

  if( npbclass != NPBClass::S ) printf( "\n   iteration\n" );

  total_time = Grappa_walltime();

  // This is the main iteration
  for( iteration=1; iteration<=MAX_ITERATIONS; iteration++ ) {
      if (npbclass != NPBClass::S ) printf( "        %d\n", iteration );
      rank( iteration );
  }

  total_time = Grappa_walltime() - total_time;
  std::cerr << "total_time: " << total_time << "\n";
}

static void parseOptions(int argc, char ** argv);

int main(int argc, char* argv[]) {
  Grappa_init(&argc, &argv);
  Grappa_activate();
  
  parseOptions(argc, argv);
  
  Grappa_run_user_main(&user_main, (void*)NULL);
  
  Grappa_finish(0);
  return 0;
}

static void printHelp(const char * exe) {
  printf("Usage: %s [options]\nOptions:\n", exe);
  printf("  --help,h   Prints this help message displaying command-line options\n");
  printf("  --scale,s  Number of keys to be sorted: 2^SCALE keys.\n");
  printf("  --log2buckets,b  Number of buckets will be 2^log2buckets.\n");
  printf("  --log2maxkey,k   Maximum value of random numbers, range will be [0,2^log2maxkey)");
  printf("  --class,c   NAS Parallel Benchmark Class (problem size) (W, S, A, B, C, or D)");
  printf("  --gen,g   Generate random numbers and save to file only (no sort).\n");
  printf("  --write,w   Save array to file.\n");
  printf("  --read,r  Read from file rather than generating.\n");
  printf("  --nosort  Skip sort.\n");
  exit(0);
}

static void parseOptions(int argc, char ** argv) {
  struct option long_opts[] = {
    {"help", no_argument, 0, 'h'},
    {"scale", required_argument, 0, 's'},
    {"log2buckets", required_argument, 0, 'b'},
    {"log2maxkey", required_argument, 0, 'k'},
    {"class", required_argument, 0, 'c'},
    {"nosort", no_argument, 0, 'n'}
  };

  ///////////////////
  // defaults
  scale = 8;

  // at least 2*num_nodes buckets by default
  nbuckets = 2;
  log2buckets = 1;
  Node nodes = Grappa_nodes();
  while (nbuckets < nodes) {
    nbuckets <<= 1;
    log2buckets++;
  }

  log2maxkey = 64;
  ///////////////////

  int c = 0;
  while (c != -1) {
    int option_index = 0;
    c = getopt_long(argc, argv, "hs:b:", long_opts, &option_index);
    switch (c) {
      case 'h':
        printHelp(argv[0]);
        exit(0);
      case 's':
        scale = atoi(optarg);
        break;
      case 'b':
        log2buckets = atoi(optarg);
        break;
      case 'k':
        log2maxkey = atoi(optarg);
        break;
      case 'c':
        npbclass = get_npb_class(optarg[0]);
        log2maxkey = MAX_KEY_LOG2[npbclass];
        log2buckets = NBUCKET_LOG2[npbclass];
        scale = NKEY_LOG2[npbclass];
        break;
    }
  }
  nkeys = 1L << scale;
  nbuckets = 1L << log2buckets;
  maxkey = (log2maxkey == 64) ? (std::numeric_limits<uint64_t>::max()) : (1ul << log2maxkey) - 1;
}

