
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <complex>
using std::complex;

#include <ForkJoin.hpp>

typedef GlobalAddress< complex<double> > complex_addr;

static int64_t N;
static int64_t log2N;

// single iteration, for single element
void fft_elt_iter(int64_t i, complex<double> * vbase, const int64_t& m) {
  const complex<double> v = vbase[i];
  

}

LOOP_FUNCTOR( setup_globals, ((int64_t,_log2N)) ) {
  log2N = _log2N;
  N = 1L<<log2N;
}

void fft(complex_addr vec, int64_t log2N) {
  { setup_globals f(log2N); fork_join_custom(&f); }

  for (int m=0; m<log2N; m++) {
    forall_local<complex<double>,fft_elt_iter>(vec, N, m);
  }
}
