
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/math/constants/constants.hpp>
const double PI = boost::math::constants::pi<double>();

#include <complex>
using std::complex;

#include <ForkJoin.hpp>
#include <Delegate.hpp>
#include <GlobalAllocator.hpp>
#include <Array.hpp>

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

  complex<double> _buf[2];
  Incoherent< complex<double> >::RO Aj(vA+j, 1, _buf+0);
  Incoherent< complex<double> >::RO Ak(vA+k, 1, _buf+1);
  Aj.start_acquire(); Ak.start_acquire();

  complex<double> result = *Aj + *Ak * std::pow(omega_n, 1L<<m);
  { Incoherent< complex<double> >::WO resultC(vB+i, 1, &result); }
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

void fft(complex_addr vec, int64_t log2N) {
  N = 1L<<log2N;
  complex_addr vtemp = Grappa_typed_malloc< complex<double> >(N);
  VLOG(1) << "vec = " << print_complex(vec) << ", vtemp = " << print_complex(vtemp);


  { setup_globals f(log2N, vec, vtemp); fork_join_custom(&f); }

  for (int64_t m=0; m < log2N; m++) {
    VLOG(1) << "vA = " << print_complex(vA) << ", vB = " << print_complex(vB);
    { char st[256]; sprintf(st,"vec  m = %d",m); print_array(st, vec, N); }
    { char st[256]; sprintf(st,"temp m = %d",m); print_array(st, vtemp, N); }
    forall_local_blocking<complex<double>,int64_t,fft_elt_iter>(vA, N, make_global(&m));
    { swap_ptrs f; fork_join_custom(&f); }
  }
  
  if (vB != vec) {
    Grappa_memcpy(vec, vB, N);
  }
  Grappa_free(vtemp);
}
