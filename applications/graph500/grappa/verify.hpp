#include "Addressing.hpp"
#include "common.h"

void compute_levels(GlobalAddress<int64_t> level, int64_t nv, GlobalAddress<int64_t> bfs_tree, int64_t root);

int64_t verify_bfs_tree(GlobalAddress<int64_t> bfs_tree, int64_t max_bfsvtx, int64_t root, tuple_graph * tg);


static int compare_doubles(const void* a, const void* b) {
  double aa = *(const double*)a;
  double bb = *(const double*)b;
  return (aa < bb) ? -1 : (aa == bb) ? 0 : 1;
}
enum {s_minimum, s_firstquartile, s_median, s_thirdquartile, s_maximum, s_mean, s_std, s_LAST};
static void get_statistics(const double x[], int n, double r[s_LAST]);
void output_results(const int64_t SCALE, int64_t nvtx_scale, int64_t edgefactor,
                    const double A, const double B, const double C, const double D,
                    const double generation_time,
                    const double construction_time,
                    const int NBFS, const double *bfs_time, const int64_t *bfs_nedge);


#define MAX_ACQUIRE_SIZE (1L<<20)

template< typename T >
inline void read_array(GlobalAddress<T> base_addr, int64_t nelem, FILE* fin) {
  int64_t bufsize = MAX_ACQUIRE_SIZE / sizeof(T);
  T* buf = new T[bufsize];
  for (int64_t i=0; i<nelem; i+=bufsize) {
    if (i % 51200 == 0) VLOG(3) << "reading " << i;
    int64_t n = std::min(nelem-i, bufsize);
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
    int64_t n = std::min(nelem-i, bufsize);
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
    int64_t n = std::min(nelem-i, bufsize);
    typename Incoherent<T>::RO c(base_addr+i, n, buf);
    c.block_until_acquired();
    fwrite(buf, sizeof(T), n, fout);
  }
  delete [] buf;
}
