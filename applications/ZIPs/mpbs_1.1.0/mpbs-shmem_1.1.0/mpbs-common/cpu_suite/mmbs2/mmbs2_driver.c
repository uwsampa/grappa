#include <bench.h>
#include "mmbs2.h"

/* VECLEN == number of uint64s stored in one vec_t */
#define VECLEN (sizeof(vec_t)/8)

/* NSTORIES == number of bits in one vec_t */
#define NSTORIES (64*VECLEN)

#if VEC_WIDTH == 32
#  warning "mmbs2 requires VEC_WIDTH of 64 or greater."
int main(int argc, char *argv[]) {
  return 0;
}
#else
int main(int argc, char *argv[]) {
  brand_t br;
  timer t;
  int64 i, j, k, m, n, p;
  int64 veclen = sizeof(vec_t)/4;
  vec_t x;
  static uint64 test[NSTORIES*TOT];
  static vec_t tmp[64] __attribute__ ((aligned (ALIGN)));
  static vec_t in[TLEN+PAD] __attribute__ ((aligned (ALIGN)));
  static vec_t out[TLEN] __attribute__ ((aligned (ALIGN)));

  n = bench_init (argc, argv, &br, &t, NULL);

  for (i = 0; i < NSTORIES*TOT; i++)
    test[i] = (brand(&br) & _maskr(63)) % ((1L<<N)-1);

  for (i = 0; i < TOT; i++) {
    for (j = 0; j < 64; j++)
      for (k = 0; k < VECLEN; k++)
        vec_set64(&tmp[j], test[64*TOT*k + 64*i + j], k);
    mmtilt(tmp);
    for (j = 0; j < N; j++)
      in[(i%NWORDS) + (i/NWORDS)*VLEN + j*NWORDS] = tmp[63-j];
    printf ("  set %ld:\n\n", i);
    for (j = 0; j < N; j++) {
      printf("%2ld ", j);
      for (k = 0; k < VECLEN; k++)
        printf(" %016lx", vec_get64(&in[(i%NWORDS) + (i/NWORDS)*VLEN + j*NWORDS], k));
      printf("\n");
    }
  }

  for (j = 0; j < (TOT/NWORDS); j++)
    mmbs2(&in[j*VLEN], &out[j*VLEN]);

  /* compute answers to check against */
  for (i = 0; i < NSTORIES*TOT; i++)
    test[i] = ((long)test[i] * (long)test[i]) % ((1L<<N)-1);

  for (i = 0; i < TOT; i++) {
    printf ("  set %ld:\n\n", i);
    for (j = 0; j < N; j++) {
      printf("%2ld ", j);
      for (k = 0; k < VECLEN; k++)
        printf(" %016lx", vec_get64(&out[(i%NWORDS) + (i/NWORDS)*VLEN + j*NWORDS], k));
      printf("\n");
    }
    for (j = 0; j < N; j++)
      tmp[63-j] = out[(i%NWORDS) + (i/NWORDS)*VLEN + j*NWORDS];
    for (; j < 64; j++)
      tmp[63-j] = vec_xor(tmp[63-j], tmp[63-j]);
    mmtilt(tmp);
    for (j = 0; j < 64; j++) {
      for (k = 0; k < VECLEN; k++) {
        m = vec_get64(&tmp[j], k);
        p = m - test[64*TOT*k + 64*i + j];
        printf("%2ld  %10ld  %10ld  %ld\n", j, m, test[64*TOT*k + 64*i + j], p);
        if (p) {
          printf("ERROR\n");
          exit(1);
        }
      }
    }
  }

  /* now our timing runs */

  timer_start (&t);
  for (k = 0; k < n; k++) {
    x = vec_set64_1(brand(&br));

    for (i = 0; i < TLEN; i++)
      in[i] = vec_xor(in[i], x);

    for (j = 0; j < (TOT/NWORDS); j++)
      mmbs2(&in[j*VLEN], &out[j*VLEN]);
  }
  timer_stop (&t);

  /* print out last result */
  i = TOT-1;
  for (j = 0; j < N; j++) {
    printf("%2ld ", j);
    for (k = 0; k < VECLEN; k++)
      printf(" %016lx", vec_get64(&out[(i%NWORDS) + (i/NWORDS)*VLEN + j*NWORDS], k));
    printf("\n");
  }

  printf("Operations: %lu, n = %lu, TOT = %lu, NSTORIES = %lu\n", n * TOT * NSTORIES, n, TOT, NSTORIES);
  printf("sizeof(vec_t) = %d\n", sizeof(vec_t));
  bench_end (&t, n*TOT*NSTORIES, "bit-serial squares");

  return 0;
}
#endif
