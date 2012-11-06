/* CVS info                         */
/* $Date: 2010/04/02 20:44:33 $     */
/* $Revision: 1.5 $                 */
/* $RCSfile: tilt.c,v $             */
/* $State: Stab $:                */

#include "bench.h"

#define M0 0x5555555555555555uL
#define M1 0x3333333333333333uL
#define M2 0x0f0f0f0f0f0f0f0fuL
#define M3 0x00ff00ff00ff00ffuL
#define M4 0x0000ffff0000ffffuL
#define M5 0x00000000ffffffffuL

#define TILTM(A,B,S,M) {\
  uint64 z = ((A) ^ ((B) >> (S))) & (M);\
  (A) ^= z;  (B) ^= (z << (S));\
}


/*@unused@*/
static char cvs_info[] = "BMARKGRP $Date: 2010/04/02 20:44:33 $ $Revision: 1.5 $ $RCSfile: tilt.c,v $ $Name:  $";

//void tilt (uint64 p[64])
//void tilt (uint64 p[const 64])
void tilt (uint64 p[])

{
  static uint64 tmp[64];
  uint64 *q;

  for (q = &tmp[0]; q < &tmp[64]; p++, q+=8) {
    uint64 a, b, c, d, e, f, g, h;

    a = p[ 0];        b = p[ 8];        c = p[16];        d = p[24];
    TILTM(a,b,8,M3);  TILTM(c,d,8,M3);  TILTM(a,c,16,M4); TILTM(b,d,16,M4);

    e = p[32];        f = p[40];        g = p[48];        h = p[56];
    TILTM(e,f,8,M3);  TILTM(g,h,8,M3);  TILTM(e,g,16,M4); TILTM(f,h,16,M4);

    TILTM(a,e,32,M5); TILTM(b,f,32,M5); TILTM(c,g,32,M5); TILTM(d,h,32,M5);
    q[ 0] = a;        q[ 1] = b;        q[ 2] = c;        q[ 3] = d;
    q[ 4] = e;        q[ 5] = f;        q[ 6] = g;        q[ 7] = h;
  }
  /* reset pointer */
  p-=8;

  for (q = &tmp[0]; q < &tmp[8]; p+=8, q++) {
    uint64 a, b, c, d, e, f, g, h;

    a = q[ 0];        b = q[ 8];        c = q[16];        d = q[24];
    TILTM(a,b,1,M0);  TILTM(c,d,1,M0);  TILTM(a,c,2,M1);  TILTM(b,d,2,M1);

    e = q[32];        f = q[40];        g = q[48];        h = q[56];
    TILTM(e,f,1,M0);  TILTM(g,h,1,M0);  TILTM(e,g,2,M1);  TILTM(f,h,2,M1);

    TILTM(a,e,4,M2);  TILTM(b,f,4,M2);  TILTM(c,g,4,M2);  TILTM(d,h,4,M2);
    p[ 0] = a;        p[ 1] = b;        p[ 2] = c;        p[ 3] = d;
    p[ 4] = e;        p[ 5] = f;        p[ 6] = g;        p[ 7] = h;
  }
}
