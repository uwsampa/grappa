
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
#include <limits>

#include <Grappa.hpp>
#include <GlobalAllocator.hpp>
#include <Cache.hpp>
#include <ForkJoin.hpp>
#include <GlobalTaskJoiner.hpp>
#include <Collective.hpp>
#include <Delegate.hpp>
#include <FileIO.hpp>
#include <Array.hpp>
#include <ParallelLoop.hpp>
#include <Metrics.hpp>
#include <AsyncDelegate.hpp>
#include <Addressing.hpp>

// XXX
#include <iostream>
#include <fstream>

using namespace Grappa;

/// Parameter for how large the file I/O buffer is
static const size_t BUFSIZE = 1L<<22;

// little helper for iterating over things numerous enough to need to be buffered
#define for_buffered(i, n, start, end, nbuf) \
  for (size_t i=start, n=nbuf; i<end && (n = std::min(nbuf, end-i)); i+=nbuf)

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



////////////
// Globals
////////////

// local version on each node, which then gets local copy of global state
// std::vector<size_t> counts;
Grappa::LocalVector<size_t> counts;
// std::vector<size_t> offsets;
Grappa::LocalVector<size_t> offsets;


template < typename S >
struct bucket_t {
  S * v;
  size_t nelems;
  size_t maxelems;
  char pad[block_size-sizeof(S*)-sizeof(size_t)*2];
  bucket_t(): v(NULL), nelems(0) { memset(pad, 0x55, sizeof(pad)); }
  ~bucket_t() { 
    if ( v ) {
      Grappa::impl::locale_shared_memory.deallocate( v ); 
    }
  }
  
  void reserve(size_t nelems) {
    if (v != NULL) {
      Grappa::impl::locale_shared_memory.deallocate( v ); 
    }
    
    maxelems = nelems;

    v = static_cast<S*>(Grappa::impl::locale_shared_memory.allocate(nelems * sizeof(S)));
    
    if (!v) {
      LOG(ERROR) << "Unable to allocate bucket of " << nelems << " elements."; exit(1);
    }
  }
  const uint64_t& operator[](size_t i) const { return v[i]; }
  S& operator[](size_t i) {
    CHECK( i < maxelems ) << "i = " << i << ", maxelems: " << maxelems;
    return v[i];
  }
  void append(S val) {
    v[nelems] = val;
    nelems++;
    CHECK(nelems <= maxelems);
  }
  size_t size() { return nelems; }
};

template< typename S >
void bucket_sort(GlobalAddress<S> array, size_t nelems, int (*Scmp)(const void*,const void*), size_t (*lobits)(S,int), int log2buckets, int log2maxkey ) {
  double t, sort_time, histogram_time, allreduce_time, scatter_time, local_sort_scatter_time, put_back_time;

  int LOBITS = log2maxkey - log2buckets;
  size_t nbuckets = 1 << log2buckets;

  GlobalAddress<bucket_t<S> > bucketlist = Grappa::global_alloc<bucket_t<S> >(nbuckets);

#ifdef DEBUG
  for (size_t i=0; i<nbuckets; i++) {
    GlobalAddress<bucket_t<S> > bi = bucketlist+i;
    VLOG(1) << "bucket[" << i << "] on Node " << bi.core() << ", offset = " << bi.pointer() - bucketlist.localize(bi.core()) << ", ptr = " << bi.pointer();
  }
#endif

  sort_time = Grappa::walltime();

  // initialize globals and histogram counts
  // { setup_counts f(array, nbuckets, bucketlist); fork_join_custom(&f); }
  on_all_cores([nbuckets]{
    counts.resize(nbuckets);
    offsets.resize(nbuckets);
    for (size_t i=0; i<nbuckets; i++) {
      counts[i] = 0;
    }
  });

  t = Grappa::walltime();

  // do local bucket counts
  // forall_local<uint64_t,histogram>(array, nelems);
  forall(array, nelems, [lobits,LOBITS](int64_t i, S& v) {
    size_t b = lobits(v, LOBITS); // TODO decide how to compare general in pieces
    counts[b]++;
  });

  histogram_time = Grappa::walltime() - t;
  LOG(INFO) << "histogram_time: " << histogram_time;
  t = Grappa::walltime();

  // allreduce everyone's counts & compute global offsets (prefix sum)
  // { aggregate_counts f; fork_join_custom(&f); }
  on_all_cores([nbuckets] {
    CHECK_EQ(nbuckets, counts.size());
    // all nodes get total counts put into their counts array
    allreduce_inplace<size_t,collective_add>(&counts[0], counts.size());

    VLOG(1) << "after allreduce_inplace (just in case)";

    // everyone computes prefix sum over buckets locally
    offsets[0] = 0;
    for (size_t i=1; i<offsets.size(); i++) {
      offsets[i] = offsets[i-1] + counts[i-1];
    }
  });
  
  allreduce_time = Grappa::walltime() - t;
  LOG(INFO) << "allreduce_time: " << allreduce_time;

  // allocate space in buckets
  VLOG(3) << "allocating space...";
  // forall_local<bucket_t,init_buckets>(bucketlist, nbuckets);
  forall(bucketlist, nbuckets, [](int64_t id, bucket_t<S>& bucket){
    // (global malloc doesn't call constructors)
    new (&bucket) bucket_t<S>();
    bucket.reserve(counts[id]);
  });
  
  VLOG(3) << "scattering...";
      t = Grappa::walltime();

  // scatter into buckets
  // forall_local<uint64_t,scatter>(array, nelems);
  forall(array, nelems, [bucketlist,lobits,LOBITS](int64_t s, int64_t n, S * first){
    size_t nbuckets = counts.size();
    
    for (int i=0; i<n; i++) {
      auto v = first[i];
      size_t b = lobits(v, LOBITS);
      CHECK( b < nbuckets ) << "bucket id = " << b << ", nbuckets = " << nbuckets;
      // ff_delegate<bucket_t,uint64_t,ff_append>(bucketlist+b, v);
      auto destb = bucketlist+b;
      delegate::call<async>(destb.core(), [destb,v]{
        destb.pointer()->append(v);
      });
    }
  });
    
  scatter_time = Grappa::walltime() - t;
  LOG(INFO) << "scatter_time: " << scatter_time;
  t = Grappa::walltime();

  // sort buckets locally
  // forall_local<bucket_t,sort_bucket>(bucketlist, nbuckets);
  /// Do some kind of local serial sort of a bucket
  forall(bucketlist, nbuckets, [Scmp](int64_t bucket_id, bucket_t<S>& bucket){
    if (bucket.size() == 0) return;
    qsort(&bucket[0], bucket.size(), sizeof(S), Scmp);
  });

  local_sort_scatter_time = Grappa::walltime() - t;
  LOG(INFO) << "local_sort_time: " << local_sort_scatter_time;  
  t = Grappa::walltime(); 
  
  // redistribute buckets back into global array  
  // forall_local<bucket_t,put_back_bucket>(bucketlist, nbuckets);
  /// Redistribute sorted buckets back into global array
  forall(bucketlist, nbuckets, [array,bucketlist](int64_t b, bucket_t<S>& bucket) {
    const size_t NBUF = BUFSIZE / sizeof(S);
    DCHECK( b < counts.size() );

    // TODO: shouldn't need to buffer this, but a bug of some sort is currently forcing us to limit the number of outstanding messages    
    //for_buffered(i, n, 0, bucket.size(), NBUF) {
    //  typename Incoherent<S>::WO c(array+offsets[b]+i, n, &bucket[i]);
    //  c.block_until_released();
    // }
    
    if ( bucket.size() > 0 ) {
      typename Incoherent<S>::WO c(array+offsets[b], bucket.size(), &bucket[0]); // FIXME v is not in locale-shared-memory
      c.block_until_released();
    }

    VLOG(3) << "bucket[" << b << "] release successful";
  });
  
  put_back_time = Grappa::walltime() - t;
  LOG(INFO) << "put_back_time: " << put_back_time;
  
  sort_time = Grappa::walltime() - sort_time;
  LOG(INFO) << "total_sort_time: " << sort_time;
}
