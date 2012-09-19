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
#include <sys/stat.h>
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

namespace grappa {
  inline int64_t read(GlobalAddress<int64_t> addr) { return SoftXMT_delegate_read_word(addr); }
  inline void write(GlobalAddress<int64_t> addr, int64_t val) { return SoftXMT_delegate_write_word(addr, val); }
  inline bool cmp_swap(GlobalAddress<int64_t> address, int64_t cmpval, int64_t newval) {
    return SoftXMT_delegate_compare_and_swap_word(address, cmpval, newval);
  }
}

static void printHelp(const char * exe);
static void parseOptions(int argc, char ** argv);


typedef boost::mt19937_64 engine_t;
typedef boost::uniform_int<uint64_t> dist_t;
typedef boost::variate_generator<engine_t&,dist_t> gen_t;

struct bucket_t {
  uint64_t * v;
  size_t sz;
  char pad[block_size-sizeof(uint64_t)-sizeof(size_t)];
  bucket_t(): v(NULL), sz(0) { memset(pad, 0x55, sizeof(pad)); }
  ~bucket_t() { delete [] v; }
  void reserve(size_t nelems) {
    if (v != NULL) delete [] v;
    v = (uint64_t*)malloc(sizeof(uint64_t)*nelems);
  }
  const uint64_t& operator[](size_t i) const { return v[i]; }
  uint64_t& operator[](size_t i) { return v[i]; }
  void append(uint64_t val) {
    v[sz] = val;
    sz++;
  }
  size_t size() { return sz; }
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
bool read_from_disk;
bool generate_and_save;

#define LOBITS (log2maxkey - log2buckets)

inline void set_random(uint64_t * v) {
  // continue using same generator with multiple calls (to not repeat numbers)
  // but start at different seed on each node so we don't get overlap
  static engine_t engine(12345L*SoftXMT_mynode());
  static gen_t gen(engine, dist_t(0, maxkey-1));
  
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
  for (size_t i=0; i<nelem; i++) ss << " " << grappa::read(base+i);
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
  *bucket = bucket_t();
  bucket->reserve(counts[id]);
}

void ff_append(bucket_t& bucket, const uint64_t& val) {
  bucket.append(val);
}

inline void scatter(uint64_t * v) {
  size_t b = (*v) >> LOBITS;
  CHECK( b < nbuckets ) << "bucket id = " << b << ", nbuckets = " << nbuckets;
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
  qsort(&(*bucket)[0], bucket->size(), sizeof(uint64_t), &ui64cmp);
}

inline void put_back_bucket(bucket_t * bucket) {
  size_t b = calc_bucket_id(bucket);
  Incoherent<uint64_t>::WO c(array+offsets[b], bucket->size(), &(*bucket)[0]);
}

void bucket_sort(GlobalAddress<uint64_t> array, size_t nelems, size_t nbuckets) {

  double t, sort_time, histogram_time, allreduce_time, scatter_time, local_sort_scatter_time, put_back_time;

  GlobalAddress<bucket_t> bucketlist = SoftXMT_typed_malloc<bucket_t>(nbuckets);

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
  VLOG(3) << "allocating space...";
  forall_local<bucket_t,resize_bucket>(bucketlist, nbuckets);
  
  VLOG(3) << "scattering...";
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
}

class BlockRecord {
  size_t start_index;
  size_t num_bytes;
  char * data;
  void save(std::fstream& f) {
    f.write((char*)&start_index, sizeof(size_t));
    f.write((char*)&num_bytes, sizeof(size_t));
    f.write(data, num_bytes);
  }
};

static const size_t BUFSIZE = 1L<<22;

// little helper for iterating over things numerous enough to need to be buffered
#define for_buffered(i, n, start, end, nbuf) \
  for (size_t i=start, n=nbuf; i<end && (n = MIN(nbuf, end-i)); i+=nbuf)

/// Save 'records' of the form:
/// { <start_index>, <num_elements>, <data...> }
template < typename T >
struct save_array_func : ForkJoinIteration {
    GlobalAddress<T> array; size_t nelems; char dirname[256];
  save_array_func() {}
  save_array_func( const char dirname[256], GlobalAddress<T> array, size_t nelems):
    array(array), nelems(nelems) { memcpy(this->dirname, dirname, 256); }
  void operator()(int64_t nid) const {
    range_t r = blockDist(0, nelems, SoftXMT_mynode(), SoftXMT_nodes());
    char fname[256]; sprintf(fname, "%s/block.%ld.%ld.gblk", dirname, r.start, r.end);
    std::fstream fo(fname, std::ios::out | std::ios::binary);
    
    const size_t NBUF = BUFSIZE/sizeof(T); 
    T * buf = new T[NBUF];
    for_buffered (i, n, r.start, r.end, NBUF) {
      typename Incoherent<T>::RO c(array+i, n, buf);
      c.block_until_acquired();
      fo.write((char*)buf, sizeof(T)*n);
    }
    delete [] buf;

    fo.close();
  }
};

template < typename T >
void save_array(const char (&dirname)[256], GlobalAddress<T> array, size_t nelems) {
  // make directory with mode 777
  if ( mkdir((const char *)dirname, 0777) != 0) {
    switch (errno) { // error occurred
      case EEXIST: LOG(INFO) << "files already exist, skipping write..."; return;
      default: fprintf(stderr, "Error with `mkdir`!\n"); break;
    }
  }

  double t = SoftXMT_walltime();

  { save_array_func<T> f(dirname, array, nelems); fork_join_custom(&f); }
  
  t = SoftXMT_walltime() - t;
  LOG(INFO) << "save_array_time: " << t;
  LOG(INFO) << "save_rate_mbps: " << ((double)nelems * sizeof(T) / (1L<<20)) / t;
}

#include <iterator>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

template < typename T >
struct read_array_func : ForkJoinIteration {
  GlobalAddress<T> array; size_t nelems; char dirname[256]; GlobalAddress<int64_t> locks;
  read_array_func() {}
  read_array_func( const char dirname[256], GlobalAddress<T> array, size_t nelems, GlobalAddress<int64_t> locks):
    array(array), nelems(nelems), locks(locks) { memcpy(this->dirname, dirname, 256); }
  void operator()(int64_t nid) const {
    range_t r = blockDist(0, nelems, SoftXMT_mynode(), SoftXMT_nodes());

    const size_t NBUF = BUFSIZE/sizeof(T);
    T * buf = new T[NBUF];

    size_t i = 0;
    fs::directory_iterator d(dirname);
    for (size_t i = 0; d != fs::directory_iterator(); i++, d++) {
      // to save time in common case, the first files go to the first nodes
      if (i < SoftXMT_mynode()) continue;

      // take assigned file, otherwise check if anyone else is reading this already
      if (i == SoftXMT_mynode() || grappa::cmp_swap(locks+i, 0, 1)) {
        const char * fname = d->path().stem().string().c_str();
        int64_t start, end;
        sscanf(fname, "block.%ld.%ld", &start, &end);
        //VLOG(1) << "reading " << start << ", " << end;
        CHECK( start < end && start < nelems && end <= nelems) << "nelems = " << nelems << ", start = " << start << ", end = " << end;
        
        std::fstream f(d->path().string().c_str(), std::ios::in | std::ios::binary);

        // read array
        for_buffered (i, n, start, end, NBUF) {
          CHECK( i+n <= nelems) << "nelems = " << nelems << ", i+n = " << i+n;
          f.read((char*)buf, sizeof(T)*n);
          typename Incoherent<T>::WO c(array+i, n, buf);
        }
      }
    }

    delete [] buf;
  }
};
template < typename T >
void read_array(const char (&dirname)[256], GlobalAddress<T> array, size_t nelems) {
  double t = SoftXMT_walltime();
  
  size_t nfiles = std::distance(fs::directory_iterator(dirname), fs::directory_iterator());
  GlobalAddress<int64_t> file_taken = SoftXMT_typed_malloc<int64_t>(nfiles);
  SoftXMT_memset_local(file_taken, (int64_t)0, nfiles);

  { read_array_func<T> f(dirname, array, nelems, file_taken); fork_join_custom(&f); }
  
  t = SoftXMT_walltime() - t;
  LOG(INFO) << "read_array_time: " << t;
  LOG(INFO) << "read_rate_mbps: " << ((double)nelems * sizeof(T) / (1L<<20)) / t;

  SoftXMT_free(file_taken);
  //const size_t hdfs_block_size = 128 * (1L<<20);

  //for (fs::director_iterator d(dirname); d != director_iterator(); d++) {
    //size_t sz = fs::file_size(d->path());

  //}
}

void user_main(void* ignore) {
  SoftXMT_reset_stats();

  double t, rand_time;

  LOG(INFO) << "### Sort Benchmark ###";
  LOG(INFO) << "nelems = (1 << " << scale << ") = " << nelems << " (" << ((double)nelems)*sizeof(uint64_t)/(1L<<30) << " GB)";
  LOG(INFO) << "nbuckets = (1 << " << log2buckets << ") = " << nbuckets;
  LOG(INFO) << "maxkey = (1 << " << log2maxkey << ") = " << maxkey;

  GlobalAddress<uint64_t> array = SoftXMT_typed_malloc<uint64_t>(nelems);

  char dirname[256]; sprintf(dirname, "/scratch/hdfs/sort/uniform.%ld.%ld", scale, log2maxkey);

  if (!read_from_disk || (read_from_disk && !fs::exists(dirname))) {
    LOG(INFO) << "generating...";
      t = SoftXMT_walltime();

    // fill vector with random 64-bit integers
    forall_local<uint64_t,set_random>(array, nelems);

      rand_time = SoftXMT_walltime() - t;
      LOG(INFO) << "fill_random_time: " << rand_time;
  }

  if (generate_and_save || (read_from_disk && !fs::exists(dirname))) {
    if (fs::exists(dirname)) fs::remove_all(dirname);

    save_array(dirname, array, nelems);

    // if just --gen specified, then we're done here...
    if (generate_and_save && !read_from_disk) return;
  }

  if (read_from_disk) {
    SoftXMT_memset_local(array, (uint64_t)0, nelems);
    read_array(dirname, array, nelems);
  }

  /////////
  // sort
  /////////
  bucket_sort(array, nelems, nbuckets);

  // verify
  size_t jump = nelems/17;
  uint64_t prev;
  for (size_t i=0; i<nelems; i+=jump) {
    prev = 0;
    for (size_t j=1; j<64 && (i+j)<nelems; j++) {
      uint64_t curr = grappa::read(array+i+j);
      CHECK( curr >= prev ) << "verify failed: prev = " << prev << ", curr = " << curr;
      prev = curr;
    }
  }

  SoftXMT_merge_and_dump_stats();
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
  printf("  --gen_and_save,g   Generate random numbers and save to file only (no sort).\n");
  printf("  --read,r  Read from file rather than generating.\n");
  exit(0);
}

static void parseOptions(int argc, char ** argv) {
  struct option long_opts[] = {
    {"help", no_argument, 0, 'h'},
    {"scale", required_argument, 0, 's'},
    {"log2buckets", required_argument, 0, 'b'},
    {"log2maxkey", required_argument, 0, 'k'},
    {"class", required_argument, 0, 'c'},
    {"gen_and_save", no_argument, 0, 'g'},
    {"read", no_argument, 0, 'r'},
  };
  
  // defaults
  generate_and_save = false;
  read_from_disk = false;
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
    npb_class cls;
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
        cls = get_npb_class(optarg[0]);
        log2maxkey = MAX_KEY_LOG2[cls];
        log2buckets = NBUCKET_LOG2[cls];
        scale = NKEY_LOG2[cls];
        break;
      case 'g':
        generate_and_save = true;
        break;
      case 'r':
        read_from_disk = true;
        break;
    }
  }
  nelems = 1L << scale;
  nbuckets = 1L << log2buckets;
  maxkey = 1L << log2maxkey;
}

