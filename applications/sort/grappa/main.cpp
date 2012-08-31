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

#define read SoftXMT_delegate_read_word
#define write SoftXMT_delegate_write_word

static void printHelp(const char * exe);
static void parseOptions(int argc, char ** argv);


typedef boost::mt19937_64 engine_t;
typedef boost::uniform_int<uint64_t> dist_t;
typedef boost::variate_generator<engine_t&,dist_t> gen_t;

////////////
// Globals
////////////
int scale;
int log2buckets;
size_t nbuckets;
std::vector<size_t> counts;
std::vector<size_t> offsets;
std::vector<uint64_t>** buckets;
size_t nlocalb;

const size_t NBUF = 1L<<20;

void fill_random(uint64_t * base, size_t nelems) {
  // continue using same generator with multiple calls (to not repeat numbers)
  // but start at different seed on each node so we don't get overlap
  static engine_t engine(12345L*SoftXMT_mynode());
  static gen_t gen(engine, dist_t(0, std::numeric_limits<uint64_t>::max()));
  
  double t;
  for (size_t i=0; i<nelems; i++) {
    base[i] = gen();
  }
}

LOOP_FUNCTOR(fill_random_func, nid, ((GlobalAddress<uint64_t>,array)) ((size_t,nelems))) {
  uint64_t * base = array.localize(), * end = (array+nelems).localize();
  fill_random(base, end-base);
}

LOOP_FUNCTOR(setup_counts, nid, ((size_t,nbuckets_)) ) {
  nbuckets = nbuckets_;
  counts.resize(nbuckets);
  offsets.resize(nbuckets);
  for (size_t i=0; i<nbuckets; i++) {
    counts[i] = 0;
  }
}

inline void histogram(uint64_t * v) {
  int lobits = 64 - log2buckets;
  size_t b = (*v) >> lobits;
  counts[b]++;
}

LOOP_FUNCTION(aggregate_counts, nid ) {
  // TODO/FIXME: do as single message rather than a number of global sync's
  // (implement a 'replicator' that is statically declared on all nodes and has methods to bcast/reduce)
  for (size_t i=0; i<nbuckets; i++) {
    counts[i] = SoftXMT_allreduce<int64_t,coll_add<int64_t>,0>(counts[i]);
  }
  offsets[0] = 0;
  for (size_t i=1; i<nbuckets; i++) {
    offsets[i] = offsets[i-1] + counts[i-1];
  }
}

inline Node bucket_to_node(int64_t bucket) {
  return bucket % SoftXMT_nodes();
}
inline int64_t bucket_on_node(int64_t bucket) {
  return bucket / SoftXMT_nodes();
}

LOOP_FUNCTION(allocate_buckets, nid) {
  range_t r = blockDist(0, nbuckets, nid, SoftXMT_nodes());
  nlocalb = r.end-r.start;
  buckets = new std::vector<uint64_t>*[nlocalb];
  for (int64_t n = nid, i=0; n<nbuckets; n+=SoftXMT_nodes(), i++) {
    buckets[i] = new std::vector<uint64_t>(0);
    buckets[i]->reserve(counts[n]);
  }
}

void ff_append(void*& bucket_id, const uint64_t& val) {
  size_t local_b = ((size_t)&bucket_id) / SoftXMT_nodes();
  buckets[local_b]->push_back(val);
}

inline void scatter(uint64_t * v) {
  int lobits = 64 - log2buckets;
  size_t b = (*v) >> lobits;
  Node remote = b % SoftXMT_nodes();
  ff_delegate<void*,uint64_t,ff_append>(make_global((void*)b, remote), *v);
}

LOOP_FUNCTION(print_buckets, nid) {
  for (size_t i=0; i<nlocalb; i++) {
    std::stringstream ss;
    ss << "buckets[" << i << "] = ";
    std::vector<uint64_t>& b = *buckets[i];
    for (size_t j=0; j<b.size(); j++) {
      ss << "  " << (b[j] >> (57-log2buckets)) << "\n";
      //ss << " " << b[j];
    }
    ss << " ]"; VLOG(1) << ss.str();
  }
}

int ui64cmp(const void * a, const void * b) {
  uint64_t * aa = (uint64_t*)a;
  uint64_t * bb = (uint64_t*)b;
  if (*aa < *bb) return -1;
  else if (*aa > *bb) return 1;
  else return 0;
}

void put_back(GlobalAddress<uint64_t> array, size_t local_bucket_id, LocalTaskJoiner * lj) {
  size_t b = SoftXMT_mynode() + local_bucket_id * SoftXMT_nodes();
  std::vector<uint64_t>& bkt = *buckets[local_bucket_id];
  {
    Incoherent<uint64_t>::WO c(array+offsets[b], bkt.size(), &bkt[0]);
  }
  lj->signal();
}


LOOP_FUNCTOR(sort_buckets, nid, ((GlobalAddress<uint64_t>,array))) {
  double t, local_sort_time = 0;

  for (size_t i=0; i<nlocalb; i++) {
    std::vector<uint64_t>& b = *buckets[i];
    qsort(&b[0], b.size(), sizeof(uint64_t), &ui64cmp);
  }
}

LOOP_FUNCTOR(put_back_all, nid, ((GlobalAddress<uint64_t>,array)) ) {
  double t, local_sort_time = 0;
  LocalTaskJoiner lj; lj.reset();

  for (size_t i=0; i<nlocalb; i++) {
    lj.registerTask();
    SoftXMT_privateTask(&put_back, array, i, &lj);
  }
  lj.wait();
}


void user_main(void* ignore) {
  size_t nelems = 1L << scale;

  LOG(INFO) << "### Sort ###";
  LOG(INFO) << "nelems = (1 << " << scale << ") = " << nelems << " (" << ((double)nelems)*sizeof(uint64_t)/(1L<<30) << " GB)";
  LOG(INFO) << "nbuckets = (1 << " << log2buckets << ") = " << nbuckets;

  GlobalAddress<uint64_t> array = SoftXMT_typed_malloc<uint64_t>(nelems);

  // fill vector with random 64-bit integers
  double rand_time;
  SOFTXMT_TIME(rand_time, {
      fill_random_func f(array, nelems); fork_join_custom(&f);
  });
  VLOG(1) << "fill_random_time: " << rand_time;

  /////////
  // sort
  /////////
  double t, sort_time, histogram_time, allreduce_time, scatter_time, local_sort_scatter_time, put_back_time;
  sort_time = SoftXMT_walltime();

    // initialize histogram counts
    { setup_counts f(nbuckets); fork_join_custom(&f); }

    // do local bucket counts
    t = SoftXMT_walltime();
      forall_local<uint64_t,histogram>(array, nelems);
    histogram_time = SoftXMT_walltime() - t;
    LOG(INFO) << "histogram_time: " << histogram_time;
    
    // allreduce everyone's counts & compute global offsets (prefix sum)
    t = SoftXMT_walltime(); 
      { aggregate_counts f; fork_join_custom(&f); }
    allreduce_time = SoftXMT_walltime() - t;
    LOG(INFO) << "allreduce_time: " << allreduce_time;

    // allocate buckets
    { allocate_buckets f; fork_join_custom(&f); }

    // scatter into buckets
    t = SoftXMT_walltime(); 
      forall_local<uint64_t,scatter>(array, nelems);
    scatter_time = SoftXMT_walltime() - t;
    LOG(INFO) << "scatter_time: " << scatter_time;

    // sort buckets locally and scatter back into original array
    t = SoftXMT_walltime(); 
      { sort_buckets f(array); fork_join_custom(&f); }
    local_sort_scatter_time = SoftXMT_walltime() - t;
    LOG(INFO) << "local_sort_time: " << local_sort_scatter_time;
    
    t = SoftXMT_walltime(); 
      { put_back_all f(array); fork_join_custom(&f); }
    put_back_time = SoftXMT_walltime() - t;
    LOG(INFO) << "put_back_time: " << put_back_time;
    
  sort_time = SoftXMT_walltime() - sort_time;

  LOG(INFO) << "total_sort_time: " << sort_time;

  // verify
  size_t jump = nelems/17;
  uint64_t prev = read(array+0);
  for (size_t i=0; i<nelems; i+=jump) {
    for (size_t j=0; j<64 && (i+j)<nelems; j++) {
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
  exit(0);
}

static void parseOptions(int argc, char ** argv) {
  struct option long_opts[] = {
    {"help", no_argument, 0, 'h'},
    {"scale", required_argument, 0, 's'},
    {"log2buckets", required_argument, 0, 'b'}
  };
  
  // defaults
  scale = 8;

  // at least 2*num_nodes buckets by default
  nbuckets = 2;
  log2buckets = 0;
  Node nodes = SoftXMT_nodes();
  while (nbuckets < nodes) {
    nbuckets <<= 1;
    log2buckets++;
  }
  nbuckets << 1; log2buckets++; 

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
        nbuckets = 1L << log2buckets;
        break;
    }
  }
}

