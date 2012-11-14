
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <complex>
using std::complex;

#include <ForkJoin.hpp>

typedef GlobalAddress< complex<double> > complex_addr;

// single iteration, for single element
void fft_elt_iter(int64_t i, complex<double> * vbase, const int64_t& m) {
  const complex<double> v = vbase[i];
  
}

void fft(complex_addr vec, int64_t nelem) {
  
  forall_local<complex<double>,split_vector>(vec, nelem);

}
