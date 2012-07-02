/* -*- mode: C; mode: folding; fill-column: 70; -*- */
/* Copyright 2010,  Georgia Institute of Technology, USA. */
/* See COPYING for license. */
#if !defined(PRNG_HEADER_)
#define PRNG_HEADER_

/** Initialze the PRNG, called in a sequential context. */
void init_random (void);

extern uint64_t userseed;
extern uint_fast32_t prng_seed[5];
extern void *prng_state;

#ifdef __MTA__
#include <mta_rng.h>
#else
#include <stdlib.h>
static void prand(int64_t n, double * v) {
  int64_t i;
  extern int64_t xmtcompat_rand_initialized;
  extern void xmtcompat_initialize_rand(void);
  if (!xmtcompat_rand_initialized) xmtcompat_initialize_rand();
  for (i = 0; i < n; ++i) {
    v[i] = drand48();
  }
}
#endif /* !defined(__MTA__) */

#endif /* PRNG_HEADER_ */
