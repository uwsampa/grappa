#include <bench.h>
#include "mm_ops.h"

#if VEC_WIDTH == 64 || VEC_WIDTH == 128

#undef M0
#undef M1
#undef M2
#undef M3
#undef M4
#undef M5
#define M0 vec_set64_1(0x5555555555555555uL)
#define M1 vec_set64_1(0x3333333333333333uL)
#define M2 vec_set64_1(0x0f0f0f0f0f0f0f0fuL)
#define M3 vec_set64_1(0x00ff00ff00ff00ffuL)
#define M4 vec_set64_1(0x0000ffff0000ffffuL)
#define M5 vec_set64_1(0x00000000ffffffffuL)

#undef TILT
#if 1   /* standard tilt */
#define TILT(A,B,S,M)  TILTM(A,B,S,M)
#else   /* tipper        */
#define TILT(A,B,S,M)  TILTM(B,A,S,M)
#endif

#undef TILTM
#define TILTM(A,B,S,M) {                                \
    vec_t z = vec_and(vec_xor(A,vec_shr64(B,S)),M);     \
    A = vec_xor(A,z); B = vec_xor(B,vec_shl64(z,S));    \
}

void mmtilt (vec_t p[64])

{
  vec_t tmp[64];
  vec_t *q;

  /* two-pass out-of-place tilt; out-of-place allows
   * for stride-1 writes; each out-of-place pass is
   * composed of three tilts in registers
   */

  /* triple pass */
  for (q = &tmp[0]; q < &tmp[64]; p++, q+=8) {
    vec_t a, b, c, d, e, f, g, h;
    
    a = p[ 0];        b = p[ 8];        c = p[16];        d = p[24];
    TILT(a,b,8,M3);   TILT(c,d,8,M3);   TILT(a,c,16,M4);  TILT(b,d,16,M4);

    e = p[32];        f = p[40];        g = p[48];        h = p[56];
    TILT(e,f,8,M3);   TILT(g,h,8,M3);   TILT(e,g,16,M4);  TILT(f,h,16,M4);

    TILT(a,e,32,M5);  TILT(b,f,32,M5);  TILT(c,g,32,M5);  TILT(d,h,32,M5);
    q[ 0] = a;        q[ 1] = b;        q[ 2] = c;        q[ 3] = d;
    q[ 4] = e;        q[ 5] = f;        q[ 6] = g;        q[ 7] = h;
  }
  /* reset pointer */
  p-=8;

  /* triple pass */
  for (q = &tmp[0]; q < &tmp[8]; p+=8, q++) {
    vec_t a, b, c, d, e, f, g, h;

    a = q[ 0];        b = q[ 8];        c = q[16];        d = q[24];
    TILT(a,b,1,M0);   TILT(c,d,1,M0);   TILT(a,c,2,M1);   TILT(b,d,2,M1);

    e = q[32];        f = q[40];        g = q[48];        h = q[56];
    TILT(e,f,1,M0);   TILT(g,h,1,M0);   TILT(e,g,2,M1);   TILT(f,h,2,M1);

    TILT(a,e,4,M2);   TILT(b,f,4,M2);   TILT(c,g,4,M2);   TILT(d,h,4,M2);
    p[ 0] = a;        p[ 1] = b;        p[ 2] = c;        p[ 3] = d;
    p[ 4] = e;        p[ 5] = f;        p[ 6] = g;        p[ 7] = h;
  }
}

#endif
