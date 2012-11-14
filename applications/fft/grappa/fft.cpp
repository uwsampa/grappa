
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

typedef GlobalAddress< complex<double> > complex_addr;

complex<double> omega(int64_t n) {
  return std::exp(complex<double>(-2*PI)/n);
}

static int64_t N;
static int64_t log2N;
static complex<double> omega_n;

static complex_addr vA;
static complex_addr vB;


// single iteration, for single element
void fft_elt_iter(int64_t li, complex<double> * vbase, const int64_t& m) {
  const complex<double> v = vbase[li];
  const int64_t i = make_linear(vbase+li)-vA;

  const int64_t rm = log2N-m,
                j = i & ~(1L<<rm),
                k = i |  (1L<<rm);

  complex<double> _buf[2];
  Incoherent< complex<double> >::RO Aj(vA+j, 1, _buf+0);
  Incoherent< complex<double> >::RO Ak(vA+k, 1, _buf+1);
  Aj.start_acquire(); Ak.start_acquire();

  complex<double> result = *Aj + *Ak * std::pow(omega_n, 1L<<m);
  { Incoherent< complex<double> >::WO resultC(vB+i, 1, &result); }
}

LOOP_FUNCTOR( setup_globals, ((int64_t,_log2N)) ((complex_addr,_vorig)) ((complex_addr,_vtemp)) ) {
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

  { setup_globals f(log2N, vec, vtemp); fork_join_custom(&f); }

  for (int m=0; m < log2N; m++) {
    forall_local_blocking<complex<double>,fft_elt_iter>(vec, N, m);
  }
  
  if (vB != vec) {
  }

  Grappa_free(vtemp);
}
