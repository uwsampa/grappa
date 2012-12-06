
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/math/constants/constants.hpp>
const double PI = boost::math::constants::pi<double>();

#include <complex>
using std::complex;

#include <bitset>

#include <ForkJoin.hpp>
#include <Delegate.hpp>
#include <GlobalAllocator.hpp>
#include <Array.hpp>

#include <fftw3.h>

template< typename T >
inline void print_array(const char * name, GlobalAddress<T> base, size_t nelem) {
  std::stringstream ss; ss << name << ": [";
  T _buf;
  for (size_t i=0; i<nelem; i++) {
    typename Incoherent<T>::RO c(base+i, 1, &_buf);
    ss << " " << *c;
  }
  ss << " ]"; VLOG(1) << ss.str();
}

typedef GlobalAddress< complex<double> > complex_addr;
#define print_complex(v) v.node() << ":" << v.pointer()

static int64_t N;
static int64_t log2N;
static complex<double> omega_n;

static complex_addr vA;
static complex_addr vB;

complex<double> omega(int64_t n) {
  return std::exp(complex<double>(0.0,-2*PI/n));
}

inline int64_t bitrev(int64_t x, char m, char r) {
  std::bitset<64> a, b(x);
  for (char i=0; i<=m; i++) { a[(r-1)-i] = b[(r-1-m)+i]; }
  return a.to_ulong();
}  

// single iteration, for single element
void fft_elt_iter(int64_t li, complex<double> * vbase, const int64_t& m) {
  const complex<double> v = vbase[li];
  const int64_t i = make_linear(vbase+li)-vA;

  const int64_t rm = log2N-m-1,
                j = i & ~(1L<<rm),
                k = i |  (1L<<rm);
  
  CHECK(i < N) << "i = " << i << ", m = " << m << ", rm = " << rm << ", vA = " << print_complex(vA) << ", vB = " << print_complex(vB);
  CHECK(j < N) << "j = " << j << ", m = " << m << ", rm = " << rm;
  CHECK(k < N) << "k = " << k << ", m = " << m << ", rm = " << rm;

  VLOG(3) << "i = " << i << ", j = " << j << ", k = " << k;

  complex<double> _buf[2];
  Incoherent< complex<double> >::RO Aj(vA+j, 1, _buf+0);
  Incoherent< complex<double> >::RO Ak(vA+k, 1, _buf+1);
  Aj.start_acquire(); Ak.start_acquire();

  complex<double> result = *Aj + *Ak * std::pow(omega_n, bitrev(i,m,log2N));

  //VLOG(1) << "i " << i << ": Aj+Ak = " << *Aj+*Ak << ", omega*2^m = " << std::pow(omega_n, 1L<<m) << ", result = " << result;
  
  int64_t index = i;
  // in last step, do bit reversal indexing to write rather than writing local
  if (m == log2N-1) { index = bitrev(i,log2N-1,log2N); }
  { Incoherent< complex<double> >::WO resultC(vB+index, 1, &result); }
}

LOOP_FUNCTOR( setup_globals, nid, ((int64_t,_log2N)) ((complex_addr,_vorig)) ((complex_addr,_vtemp)) ) {
  log2N = _log2N;
  N = 1L<<log2N;
  vA = _vorig;
  vB = _vtemp;

  omega_n = omega(N);
}

LOOP_FUNCTION( swap_ptrs, nid ) {
  complex_addr tmp = vA;
  vA = vB;
  vB = tmp;
}

/// Reorders elements with bit reversal indexing while copying from vA -> vB
void final_bitrev_reorder(complex<double> * v) {
  const int64_t i = make_linear(v)-vA;
  int64_t index = bitrev(i,log2N-1,log2N);
  { Incoherent< complex<double> >::WO resultC(vB+index, 1, v); }
}

LOOP_FUNCTION( local_ffts, nid ) {
  complex<double> * local_base_a = vA.localize(),
                  * local_end_a = (vA+N).localize();
  // complex<double> * local_base_b = vB.localize(),
  //                 * local_end_b = (vB+N).localize();

  int64_t nblock = block_size / sizeof(complex<double>);
  int64_t nlocal = local_end_a - local_base_a;

  fftw_complex * buf_in  = fftw_alloc_complex(nblock);
  fftw_complex * buf_out = fftw_alloc_complex(nblock);
  fftw_plan p = fftw_plan_dft_1d(nblock, buf_in, buf_out, FFTW_FORWARD, FFTW_ESTIMATE);

  for (int64_t i=0; i<nlocal; i+=nblock) {
    size_t n = MIN(nblock, nlocal-i);
    VLOG(1) << "local fft on " << i << " : " << i+n;

    // copy into fftw in buffer
    memcpy(buf_in, local_base_a+i, n*sizeof(fftw_complex));

    fftw_execute(p);

    // copy back into global array
    memcpy(local_base_a+i, buf_out, n*sizeof(fftw_complex));
  }

  fftw_destroy_plan(p);
  fftw_free(buf_in);
  fftw_free(buf_out);
}

void fft(complex_addr vec, int64_t log2N) {
  N = 1L<<log2N;

  // FIXME: make sure vtemp is aligned the same as `vec`
  complex_addr vtemp = Grappa_typed_malloc< complex<double> >(N);
  VLOG(2) << "vec = " << print_complex(vec) << ", vtemp = " << print_complex(vtemp);

  { setup_globals f(log2N, vec, vtemp); fork_join_custom(&f); }
  VLOG(2) << "omega_n = " << omega_n;

  for (int64_t m=0; m < log2N; m++) {
    VLOG(3) << "vA = " << print_complex(vA) << ", vB = " << print_complex(vB);

    print_array("vA", vA, N);

    int64_t nblock = block_size / sizeof(complex<double>);
    int64_t rm = log2N-m-1;
    if ((1L<<rm) >= nblock) {
      // TODO: roll these two fork_joins into one
      forall_local_blocking<complex<double>,int64_t,fft_elt_iter>(vA, N, make_global(&m));
      { swap_ptrs f; fork_join_custom(&f); }
    } else {
      break;
    }
  }
  VLOG(1) << "able to do FFT locally...";
  // print_array("vA", vA, N);
  // small enough to do local to a block
  { local_ffts f; fork_join_custom(&f); }
  // result should end up in the same array, so still in vA

  VLOG(1) << "after local FFT";
  print_array("vA", vA, N);

  forall_local<complex<double>,final_bitrev_reorder>(vA, N);

  VLOG(1) << "after bitrev";
  print_array("vB", vB, N);

  if (vB != vec) {
    Grappa_memcpy(vec, vB, N);
  }

  Grappa_free(vtemp);
}
