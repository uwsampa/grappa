#include "npb_intsort.hpp"
#include "randlc.hpp"
#include <Grappa.hpp>
#include <Communicator.hpp>
#include <Addressing.hpp>
#include <GlobalAllocator.hpp>
#include <Collective.hpp>
#include <ParallelLoop.hpp>
#include <AsyncDelegate.hpp>
#include <Array.hpp>
#include <unistd.h>
#include <getopt.h>

using namespace Grappa;

static void parseOptions(int argc, char ** argv);

/// Quick and dirty growable bucket that is padded to take up a full Grappa block so 
/// that malloc'ing a global array round-robins buckets to nodes to keep it as even
/// as possible (for small numbers of buckets this is especially important)
struct bucket_t {
  uint64_t * v;
  size_t nelems;
  size_t maxelems;
  int64_t * key_ranks;
  char pad[block_size-sizeof(uint64_t*)-sizeof(size_t)*2-sizeof(int*)];
  bucket_t(): v(nullptr), nelems(0) { memset(pad, 0x55, sizeof(pad)); }
  ~bucket_t() { if (v) { Grappa::impl::locale_shared_memory.deallocate(v); } }
  void free() {
    Grappa::impl::locale_shared_memory.deallocate(v);
    v = nullptr;
  }
  void reserve(size_t nelems) {
    maxelems = nelems;
    // if (v != NULL) delete [] v;
    if (v) { Grappa::impl::locale_shared_memory.deallocate(v); }
    
    // v = new uint64_t[nelems];
    v = static_cast<uint64_t*>(Grappa::impl::locale_shared_memory.allocate(nelems * sizeof(uint64_t)));
    
    CHECK(v) << "Unable to allocate bucket of " << nelems << " elements.";
  }
  const uint64_t& operator[](size_t i) const { return v[i]; }
  uint64_t& operator[](size_t i) {
    CHECK( i < maxelems ) << "i = " << i << ", maxelems: " << maxelems;
    return v[i];
  }
  void append(uint64_t val) {
    v[nelems] = val;
    nelems++;
    CHECK(nelems <= maxelems);
  }
  size_t size() { return nelems; }
};


//////////////////////
// Globals

static const int MIN_PROCS = 1;
static const int MAX_PROCS = 1024;

static int niterations = 10;
static const int TEST_ARRAY_SIZE = 5;

bool skip_verify;

typedef int key_t;

// global const for one sort
int scale;
int log2buckets;
int log2maxkey;
int64_t nkeys;
int nbuckets;
key_t maxkey;

#define BSHIFT (log2maxkey - log2buckets)

double my_seed;

NPBClass npbclass;

GlobalAddress<key_t> key_array;

GlobalAddress<bucket_t> bucketlist;

namespace Grappa {
  template<typename T>
  class LocalVector {
    T * _storage;
    size_t _size;
    size_t max_elem;
  public:
    LocalVector(): _storage(nullptr), _size(0), max_elem(0) {}
    ~LocalVector() {
      if (_storage) { Grappa::impl::locale_shared_memory.deallocate(_storage); }
    }
    void reserve(size_t m) {
      if (max_elem < m) {
        if (_storage) { Grappa::impl::locale_shared_memory.deallocate(_storage); }
        max_elem = m;
        _storage = static_cast<T*>(Grappa::impl::locale_shared_memory.allocate(max_elem * sizeof(T)));
      }
    }
    void resize(size_t m) {
      reserve(m);
      _size = m;
    }
    size_t size() { return _size; }
    T& operator[](const int64_t i) {
      return _storage[i];
    }
  };
};

// local version on each node, which then gets local copy of global state
// std::vector<size_t> counts;
Grappa::LocalVector<size_t> counts;
// std::vector<size_t> bucket_ranks;
Grappa::LocalVector<size_t> bucket_ranks;
// std::vector<Core> bucket_cores;

// only used on Core 0.
double total_time,
       generation_time,
       histogram_time,
       allreduce_time,
       scatter_time,
       local_rank_time,
       full_verify_time;

GlobalCompletionEvent gce;

//////////////////////

// /**********************/
// /* Partial verif info */
// /**********************/
// int64_t test_index_array[6][TEST_ARRAY_SIZE] = {
//           {48427,17148,23627,62548,4431},
//           {357773,934767,875723,898999,404505},
//           {2112377,662041,5336171,3642833,4250760},
//           {41869,812306,5102857,18232239,26860214},
//           {44172927,72999161,74326391,129606274,21736814},
//           {1317351170,995930646,1157283250,1503301535,1453734525},
//         },
//         test_rank_array[6][TEST_ARRAY_SIZE] = {
//           {0,18,346,64917,65463},
//           {1249,11698,1039987,1043896,1048018},
//           {104,17523,123928,8288932,8388264},
//           {33422937,10244,59149,33135281,99}, 
//           {61147,882988,266290,133997595,133525895},
//           {1,36538729,1978098519,2145192618,2147425337},  
//         },
//         partial_verify_vals[TEST_ARRAY_SIZE];

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

template< typename T >
inline void print_array(const char * name, T* v, size_t sz) {
  std::stringstream ss; ss << name << ": [";
  for (size_t i=0; i<sz; i++) ss << " " << v[i];
  ss << " ]"; VLOG(1) << ss.str();
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
  for (size_t i=0; i<nelem; i++) ss << " " << delegate::read(base+i);
  ss << " ]"; VLOG(1) << ss.str();
}

size_t total_bucket_allocation;

void rank(int iteration) {
  double _time;
  VLOG(1) << "iteration " << iteration;
  // key_array, nkeys already available on all cores
  
  if (iteration == 0) {
    forall<&gce,1>(bucketlist, nbuckets, [](int64_t id, bucket_t& bucket){
      // (global malloc doesn't call constructors)
      new (&bucket) bucket_t();
    });
  }
  
  // allocate space for counts, etc.
  call_on_all_cores([]{
    counts.resize(nbuckets);
    bucket_ranks.resize(nbuckets);
    for (size_t i=0; i<nbuckets; i++) {
      counts[i] = 0;
    }
    // bucket_cores.resize(nbuckets);
  });
  
  VLOG(2) << "histogramming";
  _time = Grappa::walltime();

  // histogram to find out how many fall into each bucket  
  forall<&gce>(key_array, nkeys, [](int64_t i, key_t& k){
    size_t b = k >> BSHIFT;
    counts[b]++;
  });
  
  histogram_time += Grappa::walltime() - _time;
  VLOG(2) << "allreducing";
  _time = Grappa::walltime();
  
  // allreduce everyone's counts & compute global bucket_ranks (prefix sum)
  auto bl = bucketlist;
  on_all_cores([bl]{
    bucketlist = bl; // init bucketlist on all cores
    
    // all nodes get total counts put into their counts array
    allreduce_inplace<size_t,collective_add>(&counts[0], counts.size());
    
    // everyone computes prefix sum over buckets locally
    bucket_ranks[0] = 0;
    for (size_t i=1; i<nbuckets; i++) {
      bucket_ranks[i] = bucket_ranks[i-1] + counts[i-1];
    }
    
    // Gonna try trusting Grappa's cyclic distribution to work on the Gaussian distribution...
    //
    // // Determine Redistibution of keys: accumulate the bucket size totals 
    // // till this number surpasses NUM_KEYS (which the average number of keys
    // // per processor).  Then all keys in these buckets go to processor 0.
    // // Continue accumulating again until supassing 2*NUM_KEYS. All keys
    // // in these buckets go to processor 1, etc.  This algorithm guarantees
    // // that all processors have work ranking; no processors are left idle.
    // // The optimum number of buckets, however, does not result in as high
    // // a degree of load balancing (as even a distribution of keys as is
    // // possible) as is obtained from increasing the number of buckets, but
    // // more buckets results in more computation per processor so that the
    // // optimum number of buckets turns out to be 1024 for machines tested.
    // // Note that process_bucket_distrib_ptr1 and ..._ptr2 hold the bucket
    // // number of first and last bucket which each processor will have after   
    // // the redistribution is done.
    // 
    // size_t bucket_sum_accumulator = 0;
    // size_t nkeys_per_core = nkeys / cores();
    // Core c = 0;
    // for(int i=0; i<nbuckets; i++ ) {
    //   bucket_ranks[i] = bucket_sum_accumulator;
    //   bucket_cores[i] = c; // assign this bucket to core 'c'
    //   bucket_sum_accumulator += counts[i];
    //   if (bucket_sum_accumulator > (c+1)*nkeys_per_core) {
    //     c++; // once this core's full, go up to the next one
    //   }
    // }
    // CHECK_LT(c, cores());
    // CHECK_EQ(bucket_sum_accumulator, nkeys);
    total_bucket_allocation = 0;
  });
  
  allreduce_time += Grappa::walltime() - _time;
    
  // print_array("counts", &counts[0], counts.size());
  // print_array("bucket_cores", bucket_cores);
  
  // allocate space in buckets
  forall<&gce,1>(bucketlist, nbuckets, [](int64_t id, bucket_t& bucket){
    // (global malloc doesn't call constructors)
    bucket.reserve(counts[id]);
    bucket.nelems = 0;
  });

  VLOG(2) << "scattering into buckets";
  _time = Grappa::walltime();
  
  // scatter into buckets
  forall<&gce>(key_array, nkeys, [](int64_t s, int64_t n, key_t * first){
    size_t nbuckets = counts.size();
    // char msg_buf[sizeof(Message<std::function<void(GlobalAddress<bucket_t>,key_t)>>)*n];
    // MessagePool pool(msg_buf, sizeof(msg_buf));
    
    for (int64_t i=0; i<n; i++) {
      auto v = first[i];
      size_t b = v >> BSHIFT;
      CHECK( b < nbuckets ) << "bucket id = " << b << ", nbuckets = " << nbuckets;
      // ff_delegate<bucket_t,uint64_t,ff_append>(bucketlist+b, v);
      auto destb = bucketlist+b;
      delegate::call<async,&gce>(destb.core(), [destb,v]{
        destb.pointer()->append(v);
      });
    }
  });
  
  scatter_time += Grappa::walltime() - _time;
  VLOG(2) << "ranking locally";
  _time = Grappa::walltime();
  
  // Ranking of all keys occurs in this section
  on_all_cores([iteration]{
    auto bucket_base = bucketlist.localize(), bucket_end = (bucketlist+nbuckets).localize();
    
    // size_t nlocal = 0;
    
    size_t bucket_range = 1 << BSHIFT;
    
    // auto * key_counts_storage = new int[bucket_range];
    
    // bool failed = false;
    // int nverified = 0;
    
    // for each bucket...
    for (int64_t i=0; i<bucket_end-bucket_base; i++) {
      auto& b = bucket_base[i];
      
      if (b.size() == 0) continue;
      
      b.key_ranks = new int64_t[bucket_range];
      for (int64_t j=0; j<bucket_range; j++) b.key_ranks[j] = 0;
      
      auto b_id = make_linear(&b)-bucketlist;
      CHECK_LT(b_id, nbuckets) << "bucketlist: " << bucketlist;
      size_t min_key = bucket_range * b_id,
             max_key = bucket_range * (b_id+1);
      
      // count number of instances of each key
      auto key_counts = b.key_ranks - min_key;
      
      auto b_buf = &b[0];
      for (int64_t j=0; j<b.size(); j++) {
        // VLOG(1) << "b_id = " << b_id << ", b_buf[" << j << "] = " << b_buf[j];
        DCHECK_EQ(b_buf[j] >> BSHIFT, b_id);
        DCHECK_GE(b_buf[j], min_key);
        DCHECK_LT(b_buf[j], max_key);
        key_counts[ b_buf[j] ]++;
      }
      
      // prefix_sum the counts to get ranks
      // apparently it's okay for us to not add in the global offsets
      int64_t accum = bucket_ranks[b_id];
      for (int64_t j=0; j<bucket_range; j++) {
        int64_t tmp = b.key_ranks[j];
        b.key_ranks[j] = accum;
        accum += tmp;
      }
      DCHECK_EQ(accum-bucket_ranks[b_id], b.size());
      
      // // partial verify step
      // for (int t=0; t<TEST_ARRAY_SIZE; t++) {
      //   int k = partial_verify_vals[t];
      //   if (min_key <= k && k < max_key) {
      //     VLOG(1) << "partial_verify_vals[" << t << "] = " << k << ", < " << min_key << " - " << max_key << " >";
      //     auto key_rank = key_counts[k]; // + bucket_ranks[b_id];
      //     switch (npbclass) {
      //     case NPBClass::S:
      //       nverified++;
      //       if (t<=2) {
      //         // CHECK_EQ(key_rank, test_rank_array[npbclass][t]+iteration);
      //         if (key_rank != test_rank_array[npbclass][t]+iteration) failed = true;              
      //       } else {
      //         // CHECK_EQ(key_rank, test_rank_array[npbclass][t]+iteration);
      //         if (key_rank != test_rank_array[npbclass][t]-iteration) failed = true;
      //       }
      //       break;
      //     default:
      //       fprintf(stderr, "warning: skipping partial verification\n");
      //     }
      //   }
      //   if (failed) {
      //     // fprintf(stderr, "Failed partial verification: (iteration %d, t = %d)\n", iteration, t);
      //   }
      // }
    }
    // if (!failed) {
    //   fprintf(stderr, "Passed partial verification (%d).\n", nverified);
    // }    
  });
  
  local_rank_time += Grappa::walltime() - _time;
}

void full_verify() {
  VLOG(1) << "starting verify...";
  auto sorted_keys = Grappa::global_alloc<key_t>(nkeys);
  Grappa::memset(sorted_keys, -1, nkeys);
  
  forall<&gce,1>(bucketlist, nbuckets, [sorted_keys](int64_t b_id, bucket_t& b){
    auto key_ranks = b.key_ranks - (b_id<<BSHIFT);
    
    for (int64_t i=0; i<b.size(); i++) {
      key_t k = b[i];
      CHECK_LT(key_ranks[k], nkeys);
      delegate::write<async,&gce>(sorted_keys+key_ranks[k], k);
      key_ranks[k]++;
    }
  });
  
  // print_array("sorted_keys", sorted_keys, nkeys);
  
  forall(sorted_keys, nkeys-1, [](int64_t i, key_t& k){
    key_t o = delegate::read(make_linear(&k)+1);
    CHECK_LE(k, o) << "sorted_keys[" << i << ":" << i+1 << "] = " << k << ", " << o;
  });
  VLOG(1) << "done";
}

int main(int argc, char* argv[]) {
  Grappa::init(&argc, &argv);
  
  parseOptions(argc, argv);
  
  Grappa::run([]{
    int             i, iteration, itemp;
  
    //  Printout initial NPB info 
    printf( "NAS Parallel Benchmarks 3.3 -- IS Benchmark in Grappa\n");
    printf( "nkeys:       %ld  (class %c, 2^%d)\n", (long)nkeys, npb_class_char(npbclass), scale);
    printf( "maxkey:      %d (2^%d)\n", maxkey, log2maxkey);
    printf( "niterations: %d\n", niterations);
    printf( "cores:       %d\n", cores());

    // Check to see whether total number of processes is within bounds.
    if( cores() < MIN_PROCS || cores() > MAX_PROCS) {
        printf( "\n ERROR: number of processes %d not within range %d-%d"
               "\n Exiting program!\n\n", cores(), MIN_PROCS, MAX_PROCS);
        return;
    }

    auto _key_array = Grappa::global_alloc<key_t>(nkeys);

    // Gonna try trusting Grappa's cyclic distribution to work on the Gaussian distribution...  
    auto _bucketlist = Grappa::global_alloc<bucket_t>(nbuckets);

    generation_time = Grappa::walltime();

    // initialize all cores
    call_on_all_cores([_key_array, _bucketlist]{
      init_seed();
      key_array = _key_array;
      bucketlist = _bucketlist;
    });

    // Generate random number sequence and subsequent keys on all procs
    // Note: the distribution should be roughly Gaussian
    forall(key_array, nkeys, [](int64_t i, key_t& key){
      key = next_seq_element();
    });
  
    // on_all_cores([]{
    //   // Determine where the partial verify test keys are, load into top of array bucket_size
    //   for (int i=0; i<TEST_ARRAY_SIZE; i++) {
    //     partial_verify_vals[i] = delegate::read(key_array+test_index_array[npbclass][i]);
    //   }
    // });
  
    generation_time = Grappa::walltime() - generation_time;
    std::cerr << "generation_time: " << generation_time << "\n";

    // Do one interation for free (i.e., untimed) to guarantee initialization of  
    // all data and code pages and respective tables
    rank(0);
  
    if( npbclass != NPBClass::S ) printf( "\n   iteration\n" );

    histogram_time = allreduce_time = scatter_time = local_rank_time = 0;
  
    Metrics::reset_all_cores();

    Metrics::start_tracing();
  
    total_time = Grappa::walltime();

    // This is the main iteration
    for( iteration=1; iteration<=niterations; iteration++ ) {
        if (npbclass != NPBClass::S ) printf( "        %d\n", iteration );
        rank( iteration );
    }

    total_time = Grappa::walltime() - total_time;

    Metrics::stop_tracing();
    Metrics::merge_and_print();
  
    std::cerr << "total_time: " << total_time << "\n";
    std::cerr << "histogram_time: " << histogram_time << "\n";
    std::cerr << "allreduce_time: " << allreduce_time << "\n";
    std::cerr << "scatter_time: " << scatter_time << "\n";
    std::cerr << "local_rank_time: " << local_rank_time << "\n";

    if (!skip_verify) {
      full_verify_time = Grappa::walltime();
      full_verify();
      full_verify_time = Grappa::walltime() - full_verify_time;
      std::cerr << "full_verify_time: " << full_verify_time << "\n";
    }
  
    std::cerr << "problem_size: " << nkeys << "\n";
  
    double mops = static_cast<double>(niterations)*nkeys/total_time/1e6;
    std::cerr << "mops_total: " << mops << "\n";
    std::cerr << "mops_per_process: " << mops/cores() << "\n";
  });
  Grappa::finalize();
}

static void printHelp(const char * exe) {
  printf("Usage: %s [options]\nOptions:\n", exe);
  printf("  --help,h   Prints this help message displaying command-line options\n");
  printf("  --scale,s  Number of keys to be sorted: 2^SCALE keys.\n");
  printf("  --log2buckets,b  Number of buckets will be 2^log2buckets.\n");
  printf("  --log2maxkey,k   Maximum value of random numbers, range will be [0,2^log2maxkey)");
  printf("  --class,c   NAS Parallel Benchmark Class (problem size) (W, S, A, B, C, or D)");
  printf("  --niterations  Number of iterations to do.\n");
  printf("  --skip-verify Skip verification step.\n");
  exit(0);
}

static void parseOptions(int argc, char ** argv) {
  struct option long_opts[] = {
    {"help", no_argument, 0, 'h'},
    {"scale", required_argument, 0, 's'},
    {"log2buckets", required_argument, 0, 'b'},
    {"log2maxkey", required_argument, 0, 'k'},
    {"class", required_argument, 0, 'c'},
    {"niterations", required_argument, 0, 'n'},
    {"skip-verify", no_argument, 0, 'f'}
  };

  ///////////////////
  // defaults
  scale = 8;

  // at least 2*num_nodes buckets by default
  nbuckets = 2;
  log2buckets = 1;
  Core nodes = Grappa::cores();
  while (nbuckets < nodes) {
    nbuckets <<= 1;
    log2buckets++;
  }

  log2maxkey = 64;
  skip_verify = false;
  niterations = 10;
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
      case 'f':
        skip_verify = true;
        break;
      case 'n':
        niterations = atoi(optarg);
        break;      
    }
  }
  nkeys = 1L << scale;
  nbuckets = 1L << log2buckets;
  maxkey = (log2maxkey == 64) ? (std::numeric_limits<key_t>::max()) : (1ul << log2maxkey) - 1;
}

