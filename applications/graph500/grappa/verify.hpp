#include "Addressing.hpp"
#include "common.h"

void compute_levels(GlobalAddress<int64_t> level, int64_t nv, GlobalAddress<int64_t> bfs_tree, int64_t root);

int64_t verify_bfs_tree(GlobalAddress<int64_t> bfs_tree, int64_t max_bfsvtx, int64_t root, tuple_graph * tg);

#define MAX_ACQUIRE_SIZE (1L<<20)

template< typename T >
inline void read_array(GlobalAddress<T> base_addr, int64_t nelem, FILE* fin) {
  int64_t bufsize = MAX_ACQUIRE_SIZE / sizeof(T);
  T* buf = new T[bufsize];
  for (int64_t i=0; i<nelem; i+=bufsize) {
    if (i % 51200 == 0) VLOG(3) << "reading " << i;
    int64_t n = MIN(nelem-i, bufsize);
    typename Incoherent<T>::WO c(base_addr+i, n, buf);
    c.block_until_acquired();
    size_t nread = fread(buf, sizeof(T), n, fin);
    CHECK(nread == n) << nread << " : " << n;
    c.block_until_released();
  }
  delete [] buf;
}

template< typename T >
inline void read_array(GlobalAddress<T> base_addr, int64_t nelem, T* array) {
  int64_t bufsize = MAX_ACQUIRE_SIZE / sizeof(T);
  T* buf = new T[bufsize];
  for (int64_t i=0; i<nelem; i+=bufsize) {
    if (i % 51200 == 0) VLOG(3) << "sending " << i;
    int64_t n = MIN(nelem-i, bufsize);
    VLOG(5) << "buf = " << buf;
    typename Incoherent<T>::WO c(base_addr+i, n, buf);
    c.block_until_acquired();
    for (int64_t j=0; j<n; j++) c[j] = array[i+j];
    c.block_until_released();
  }
  delete [] buf;
}

template< typename T >
inline void write_array(GlobalAddress<T> base_addr, int64_t nelem, FILE* fout) {
  int64_t bufsize = MAX_ACQUIRE_SIZE / sizeof(T);
  T * buf = new T[bufsize];
  for (int64_t i=0; i<nelem; i+=bufsize) {
    int64_t n = MIN(nelem-i, bufsize);
    typename Incoherent<T>::RO c(base_addr+i, n, buf);
    c.block_until_acquired();
    fwrite(buf, sizeof(T), n, fout);
  }
  delete [] buf;
}
