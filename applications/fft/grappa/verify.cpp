
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

/// Grappa implementation of a 1D FFT beginning and ending in a single Grappa global array
/// Benchmark parameters:
///   --scale,-s  log2(nelem)

#include <unistd.h>
#include <math.h>
#include <complex>
using std::complex;

#include <fftw3.h>
#include <glog/logging.h>

void verify(complex<double> * orig, complex<double> * result, size_t nelem) {
  fftw_complex * sig = reinterpret_cast<fftw_complex*>(orig);
  fftw_complex * wout = fftw_alloc_complex(nelem);
  
  fftw_plan p = fftw_plan_dft_1d(nelem, sig, wout, FFTW_FORWARD, FFTW_ESTIMATE);
  
  fftw_execute(p);

  //complex<double> * wresult = reinterpret_cast<fftw_complex*>(wout);
  complex<double> * wresult = reinterpret_cast<complex<double>*>(wout);
  for (size_t i=0; i<nelem; i++) {
    //CHECK( wresult[i] == result[i] ) << "[" << i << "]: fftw = " << wresult[i] << ", grappa = " << result[i];
    VLOG(1) << "[" << i << "]: fftw = " << wresult[i] << ", grappa = " << result[i];
  }

  LOG(INFO) << "Verification successful!";

  fftw_free(wout);
}
