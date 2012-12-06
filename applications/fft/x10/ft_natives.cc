#include"hpccfft.h"
#include <math.h>
#include <stdint.h>

/***
    FFT_HOME=$HOME/fftw/fftw-3.1.2/
    slibclean; xlc_r -O3 -qnolm -bM:SHR -bnoentry -bexpall -bmaxdata:0x80000000 -I$JAVA_HOME/bin/include -I$JAVA_HOME/include -o libxsupport.a xsupport.c -I$FFT_HOME/api -L$FFT_HOME/.libs -l fftw3 -lm
    FFT_HOME=$HOME/fftw/fftw-3.1.2_64/
    slibclean; xlc_r -q64 -O3 -qnolm -bM:SHR -bnoentry -bexpall -blpdata -bmaxdata:0x8000000000 -I$JAVA_HOME/bin/include -I$JAVA_HOME/include -o libxsupport_64.a xsupport.c -I$FFT_HOME/api -L$FFT_HOME.libs -l fftw3 -lm
    (C) Copyright IBM Corp. 2006
***/

extern "C" {

int64_t create_plan(signed int SQRTN, signed int direction, signed int flags) {
  return (int64_t)HPCC_fftw_create_plan(SQRTN, (fftw_direction) direction, flags);
}

/* row-major array indexing */
#define SUB(A, i, j) (A)[(i)*SQRTN+(j)]

/* transform row i, for i0 <= i < i1 */
void execute_plan(int64_t plan, double* A_x10PoInTeR, double* B_x10PoInTeR, signed int SQRTN, signed int i0, signed int i1) {
  int i;
  hpcc_fftw_plan p = (hpcc_fftw_plan) plan;
  fftw_complex *A = (fftw_complex *) A_x10PoInTeR;
  fftw_complex *B = (fftw_complex *) B_x10PoInTeR;

  for (i = i0; i < i1; ++i) {
    HPCC_fftw_one(p, &SUB(A, i, 0), &SUB(B, i, 0));
  }
}

}
