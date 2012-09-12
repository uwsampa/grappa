//
//  main.c
//  AppSuite
//
//  Created by Brandon Holt on 12/13/11.
//  Copyright 2011 University of Washington. All rights reserved.
//
#if HAVE_UNISTD_H
#include <unistd.h>
#else
#if !defined(__MTA__)
#include <getopt.h>
#endif
#endif

#include <stdio.h>
#include <math.h>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>
#include <limits>

#include <SoftXMT.hpp>
#include <GlobalAllocator.hpp>
#include <Cache.hpp>
#include <ForkJoin.hpp>
#include <GlobalTaskJoiner.hpp>
#include <Collective.hpp>
#include <Delegate.hpp>

#include "npb_intsort.h"

#define read SoftXMT_delegate_read_word
#define write SoftXMT_delegate_write_word

static void printHelp(const char * exe);
static void parseOptions(int argc, char ** argv);


typedef boost::mt19937_64 engine_t;
typedef boost::uniform_int<uint64_t> dist_t;
typedef boost::variate_generator<engine_t&,dist_t> gen_t;

struct bucket_t {
  std::vector<uint64_t> b;
  char pad[block_size-sizeof(b)];
};

////////////
// Globals
////////////
int scale;
int log2buckets;
int log2maxkey;
size_t nelems;
size_t nbuckets;
uint64_t maxkey;
std::vector<size_t> counts;
std::vector<size_t> offsets;
GlobalAddress<bucket_t> bucketlist;
GlobalAddress<uint64_t> array;

#define LOBITS (log2maxkey - log2buckets)

inline void set_random(uint64_t * v) {
  // continue using same generator with multiple calls (to not repeat numbers)
  // but start at different seed on each node so we don't get overlap
  static engine_t engine(12345L*SoftXMT_mynode());
  static gen_t gen(engine, dist_t(0, maxkey));
  
  *v = gen();
}

LOOP_FUNCTOR(setup_counts, nid, ((GlobalAddress<uint64_t>,_array)) ((size_t,_nbuckets)) ((GlobalAddress<bucket_t>,_buckets)) ) {
  array = _array;
  nbuckets = _nbuckets;
  bucketlist = _buckets;

  counts.resize(nbuckets);
  offsets.resize(nbuckets);
  for (size_t i=0; i<nbuckets; i++) {
    counts[i] = 0;
  }
}

inline void histogram(uint64_t * v) {
  size_t b = (*v) >> LOBITS;
  counts[b]++;
}

template< typename T >
inline void print_array(const char * name, std::vector<T> v) {
  std::stringstream ss; ss << name << ": [";
  for (size_t i=0; i<v.size(); i++) ss << " " << v[i];
  ss << " ]"; VLOG(1) << ss.str();
}
template< typename T >
inline void print_array(const char * name, GlobalAddress<T> base, size_t nelem) {
  std::stringstream ss; ss << name << ": [";
  for (size_t i=0; i<nelem; i++) ss << " " << read(base+i);
  ss << " ]"; VLOG(1) << ss.str();
}

LOOP_FUNCTION(aggregate_counts, nid ) {
  
  SoftXMT_allreduce<size_t,coll_add<size_t>,0>(&counts[0], nbuckets);

  offsets[0] = 0;
  for (size_t i=1; i<nbuckets; i++) {
    offsets[i] = offsets[i-1] + counts[i-1];
  }
}

inline size_t calc_bucket_id(bucket_t * bucket) {
  return make_linear(bucket) - bucketlist;
}

inline void resize_bucket(bucket_t * bucket) {
  size_t id = calc_bucket_id(bucket);
  bucket->b.reserve(counts[id]);
}

void ff_append(bucket_t& bucket, const uint64_t& val) {
  bucket.b.push_back(val);
}

inline void scatter(uint64_t * v) {
  size_t b = (*v) >> LOBITS;
  ff_delegate<bucket_t,uint64_t,ff_append>(bucketlist+b, *v);
}

int ui64cmp(const void * a, const void * b) {
  uint64_t * aa = (uint64_t*)a;
  uint64_t * bb = (uint64_t*)b;
  if (*aa < *bb) return -1;
  else if (*aa > *bb) return 1;
  else return 0;
}

inline void sort_bucket(bucket_t * bucket) {
  qsort(&bucket->b[0], bucket->b.size(), sizeof(uint64_t), &ui64cmp);
}

inline void put_back_bucket(bucket_t * bucket) {
  size_t b = calc_bucket_id(bucket);
  Incoherent<uint64_t>::WO c(array+offsets[b], bucket->b.size(), &bucket->b[0]);
}


void user_main(void* ignore) {
  double t, rand_time, sort_time, histogram_time, allreduce_time, scatter_time, local_sort_scatter_time, put_back_time;

  LOG(INFO) << "### Sort ###";
  LOG(INFO) << "nelems = (1 << " << scale << ") = " << nelems << " (" << ((double)nelems)*sizeof(uint64_t)/(1L<<30) << " GB)";
  LOG(INFO) << "nbuckets = (1 << " << log2buckets << ") = " << nbuckets;
  LOG(INFO) << "maxkey = (1 << " << log2maxkey << ") = " << maxkey;

  GlobalAddress<uint64_t> array = SoftXMT_typed_malloc<uint64_t>(nelems);
  GlobalAddress<bucket_t> bucketlist = SoftXMT_typed_malloc<bucket_t>(nbuckets);

      t = SoftXMT_walltime();

  // fill vector with random 64-bit integers
  forall_local<uint64_t,set_random>(array, nelems);

      rand_time = SoftXMT_walltime() - t;
      LOG(INFO) << "fill_random_time: " << rand_time;

  /////////
  // sort
  /////////
      sort_time = SoftXMT_walltime();

  // initialize globals and histogram counts
  { setup_counts f(array, nbuckets, bucketlist); fork_join_custom(&f); }

      t = SoftXMT_walltime();

  // do local bucket counts
  forall_local<uint64_t,histogram>(array, nelems);

      histogram_time = SoftXMT_walltime() - t;
      LOG(INFO) << "histogram_time: " << histogram_time;
    
      t = SoftXMT_walltime();

  // allreduce everyone's counts & compute global offsets (prefix sum)
  { aggregate_counts f; fork_join_custom(&f); }
    
      allreduce_time = SoftXMT_walltime() - t;
      LOG(INFO) << "allreduce_time: " << allreduce_time;

  // allocate space in buckets
  forall_local<bucket_t,resize_bucket>(bucketlist, nbuckets);
    
      t = SoftXMT_walltime();

  // scatter into buckets
  forall_local<uint64_t,scatter>(array, nelems);
    
      scatter_time = SoftXMT_walltime() - t;
      LOG(INFO) << "scatter_time: " << scatter_time;

      t = SoftXMT_walltime();

  // sort buckets locally
  forall_local<bucket_t,sort_bucket>(bucketlist, nbuckets);

      local_sort_scatter_time = SoftXMT_walltime() - t;
      LOG(INFO) << "local_sort_time: " << local_sort_scatter_time;
  
      t = SoftXMT_walltime(); 
  
  // redistribute buckets back into global array  
  forall_local<bucket_t,put_back_bucket>(bucketlist, nbuckets);
    
      put_back_time = SoftXMT_walltime() - t;
      LOG(INFO) << "put_back_time: " << put_back_time;
  
      sort_time = SoftXMT_walltime() - sort_time;
      LOG(INFO) << "total_sort_time: " << sort_time;

  //print_array("array (sorted)", array, nelems);

  // verify
  size_t jump = nelems/17;
  uint64_t prev;
  for (size_t i=0; i<nelems; i+=jump) {
    prev = 0;
    for (size_t j=1; j<64 && (i+j)<nelems; j++) {
      uint64_t curr = read(array+i+j);
      CHECK( curr >= prev ) << "verify failed: prev = " << prev << ", curr = " << curr;
      prev = curr;
    }
  }
}

int main(int argc, char* argv[]) {
  SoftXMT_init(&argc, &argv);
  SoftXMT_activate();
  
  parseOptions(argc, argv);
  
  SoftXMT_run_user_main(&user_main, (void*)NULL);
  
  SoftXMT_finish(0);
  return 0;
}

static void printHelp(const char * exe) {
  printf("Usage: %s [options]\nOptions:\n", exe);
  printf("  --help,h   Prints this help message displaying command-line options\n");
  printf("  --scale,s  Scale of the graph: 2^SCALE vertices.\n");
  printf("  --log2buckets,b  Number of buckets will be 2^log2buckets.\n");
  printf("  --log2maxkey,k   Maximum value of random numbers, range will be [0,2^log2maxkey)");
  printf("  --class,c   NAS Parallel Benchmark Class (problem size) (W, S, A, B, C, or D)");
  exit(0);
}

static void parseOptions(int argc, char ** argv) {
  struct option long_opts[] = {
    {"help", no_argument, 0, 'h'},
    {"scale", required_argument, 0, 's'},
    {"log2buckets", required_argument, 0, 'b'},
    {"log2maxkey", required_argument, 0, 'k'},
    {"class", required_argument, 0, 'c'}
  };
  
  // defaults
  scale = 8;

  // at least 2*num_nodes buckets by default
  nbuckets = 2;
  log2buckets = 1;
  Node nodes = SoftXMT_nodes();
  while (nbuckets < nodes) {
    nbuckets <<= 1;
    log2buckets++;
  }

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
        npb_class cls = get_npb_class(optarg[0]);
        log2maxkey = MAX_KEY_LOG2[cls];
        log2buckets = NBUCKET_LOG2[cls];
        scale = NKEY_LOG2[cls];
        break;
    }
  }
  nelems = 1L << scale;
  nbuckets = 1L << log2buckets;
  maxkey = 1L << log2maxkey;
}

