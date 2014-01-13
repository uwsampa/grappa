#include <glog/logging.h>
#include "sort.hpp"

using namespace Grappa;

int ui64cmp(const void * a, const void * b) {
  uint64_t * aa = (uint64_t*)a;
  uint64_t * bb = (uint64_t*)b;
  if (*aa < *bb) return -1;
  else if (*aa > *bb) return 1;
  else return 0;
}

size_t lobits(uint64_t val, int nb) {
  return val >> nb;
}

struct KeyedTuple {
  uint64_t key;
  char val[64-sizeof(uint64_t)]; // HACK: block_size=64
};

int kt_cmp(const void * a, const void * b) {
  KeyedTuple * aa = (KeyedTuple*) a;
  KeyedTuple * bb = (KeyedTuple*) b;
  if (aa->key < bb->key) return -1;
  else if (aa->key > bb->key) return 1;
  else return 0;
}

size_t lobits(KeyedTuple val, int nb) {
  return val.key >> nb;
}

std::ostream& operator<<(std::ostream& o, const KeyedTuple& kt) {
  return o << "{" << kt.key << ": " << kt.val << "}";
}
  

template< typename T >
inline void print_array(const char * name, GlobalAddress<T> base, size_t nelem) {
  std::stringstream ss; ss << name << ": [";
  for (size_t i=0; i<nelem; i++) ss << " " << delegate::read(base+i);
  ss << " ]"; VLOG(1) << ss.str();
}

int main(int argc, char* argv[]) {
  Grappa::init(&argc, &argv);
  Grappa::run([]{
    LOG(INFO) << "System block_size=" << block_size;

    // test S = uint64_t
    size_t nelems = 256;
    int log2buckets = 5;
    int log2maxkey = 10;

    GlobalAddress<uint64_t> array = Grappa::global_alloc<uint64_t>( nelems );
    forall(array, nelems, [nelems,log2maxkey](int64_t i, uint64_t& e) {
      e = (nelems-i) % log2maxkey;
    });
    print_array("before-sort", array, nelems);
 

    bucket_sort<uint64_t>(array, nelems, &ui64cmp, &lobits, log2buckets, log2maxkey);
    print_array("after-sort", array, nelems);

    // verify
    size_t jump = nelems/17;
    uint64_t prev;
    for (size_t i=0; i<nelems; i+=jump) {
      prev = 0;
      for (size_t j=1; j<64 && (i+j)<nelems; j++) {
        uint64_t curr = delegate::read(array+i+j);
        CHECK( curr >= prev ) << "verify failed: prev = " << prev << ", curr = " << curr;
        prev = curr;
      }
    }
    Grappa::global_free( array );



    // test S = keyed-tuple
    GlobalAddress<KeyedTuple> tuples = Grappa::global_alloc<KeyedTuple>( nelems );
    forall(tuples, nelems, [nelems,log2maxkey](int64_t i, KeyedTuple& e) {
      e.key = (nelems-i) % log2maxkey;
      CHECK( sprintf( e.val, "S-%lu", e.key ) > 0 );
    });
    print_array("before-sort", tuples, nelems);
  
    bucket_sort<KeyedTuple>(tuples, nelems, &kt_cmp, &lobits, log2buckets, log2maxkey);
    print_array("after-sort", tuples, nelems);

    Grappa::global_free( tuples );

  });
  Grappa::finalize();
}
