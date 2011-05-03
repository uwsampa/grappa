#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/timeb.h>
#include "../bench/graph.h"

static uint64_t get_ns()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);

  uint64_t ns = 0;
  ns += ts.tv_nsec;
  ns += ts.tv_sec * (1000LL * 1000LL * 1000LL);
  return ns;
}


static unsigned int goodseed() {
  FILE *urandom = fopen("/dev/urandom", "r");
  unsigned int seed;
  assert(1 == fread(&seed, sizeof(unsigned int), 1, urandom));
  fclose(urandom);
  if (seed == 0) seed = 1;
  return seed;
}

double *genvals(graph *g) {
  double *vals = calloc(g->v, sizeof(double));
  assert (vals != NULL);
  for (unsigned int i = 0; i < g->v; ++i) {
    assert (g->row_ptr[i+1] > g->row_ptr[i]);
    vals[i] = i;
    vals[i] /= g->v;
  }
  return vals;
}

// returns uniformly chosen integer in [i,j)
static int32_t rand_range(int32_t i, int32_t j, struct random_data *gen) {
  int32_t spread = j - i;
  int32_t limit = RAND_MAX - RAND_MAX % spread;
  int32_t rnd;
  do {
    assert(0 == random_r(gen, &rnd));
    //rnd = random();
  } while (rnd >= limit);
  rnd %= spread;
  return (rnd + i);
}

#define RSIZE 1024

void genset(int32_t *state, int32_t **fptr, int32_t **rptr, int32_t *end,
            int *randoms) {
  int32_t val;
  for (int i = 0; i < RSIZE; ++i) {
    val = **fptr += **rptr;
    randoms[i] = (val >> 1) & 0x7fffffff;
    //    assert(result == random());
    ++*fptr;
    if (*fptr >= end) {
      *fptr = state;
      ++*rptr;
    } else {
      ++*rptr;
      if (*rptr >= end) {
        *rptr = state;
      }
    }
  }
}

double walk(graph *g, double *vals, int pathlen, int seed, int *left) {
  double res = 0;
  int32_t state[31];
  state[0] = seed == 0 ? 1 : seed;
  int32_t word = state[0];
  for (int i = 1; i < 31; ++i) {
    long int hi = word / 127773;
    long int lo = word % 127773;
    word = 16807 * lo - 2836 * hi;
    if (word < 0)
      word += 2147483647;
    state[i] = word;
  }
  int32_t *fptr = &state[3];
  int32_t *rptr = &state[0];
  int32_t *end = &state[31]; // one off the end
  int32_t val;
  for (int i = 0; i < 310; ++i) {
    val = *fptr += *rptr;
    ++fptr;
    if (fptr >= end) {
      fptr = state;
      ++rptr;
    } else {
      ++rptr;
      if (rptr >= end) {
        rptr = state;
      }
    }
  }

  int randoms[RSIZE];

  int rleft = 0;
  int chunk;
  int32_t limit, spread, rnd;
  while ((chunk = __sync_fetch_and_sub(left, 100)) > 0) {
    while (chunk-- > 0) {
      uint64_t vertex;
      spread = g->v;
      limit = RAND_MAX - RAND_MAX % spread;
      do {
        if (rleft == 0) {
          genset(state, &fptr, &rptr, end, randoms);
          rleft = RSIZE;
        }
        rnd = randoms[--rleft];
      } while (rnd >= limit);
      vertex = rnd % spread;
      for (int i = 0; i < pathlen; ++i) {
        spread = g->row_ptr[vertex+1] - g->row_ptr[vertex];
        int j = g->row_ptr[vertex];
        limit = RAND_MAX - RAND_MAX % spread;
        do {
          if (rleft == 0) {
            genset(state, &fptr, &rptr, end, randoms);
            rleft = RSIZE;
          }
          rnd = randoms[--rleft];
        } while (rnd >= limit);
        j += rnd % spread;
        //printf("%d: %lu %d\n", i, vertex, j);
        vertex = g->edges[j];
      }
      res += vals[vertex];
    }
  }
  return res;
}

double rwalk(graph *g, double *vals, int pathlen, int ncores, int samples) {
  int left = samples;
  double result = 0;
  uint64_t timings[ncores];
  uint64_t seeds[ncores];
  double results[ncores];
  for (int i = 0; i < ncores; ++i) {
    seeds[i] = random();
  }
  #pragma omp parallel for num_threads(ncores)
  for (int c = 0; c < ncores; ++c) {
    double locres = 0;
    uint64_t locb = get_ns();
    locres = walk(g, vals, pathlen, seeds[c], &left);
    uint64_t loca = get_ns();
    timings[c] = loca - locb;
    results[c] = locres;
  }
  uint64_t slow = 0, fast = -1;
  for (int i = 0; i < ncores; ++i) {
    result += results[i];
    printf("timing %d %d: %lu\n", ncores, i, timings[i]);
    if (slow < timings[i]) slow = timings[i];
    if (fast > timings[i]) fast = timings[i];
  }
  double imbalance = slow;
  imbalance /= fast;
  printf("imbalance: %d %f\n", ncores, imbalance);
  return result;
}

int main(int argc, char *argv[]) {
  if (argc != 6 && argc != 7) {
	printf("usage: %s file ncores-min ncores-max pathlen samples seed\n"
               "file     - input graph file\n"
               "ncores-min, ncores-max   - number of hardware threads\n"
               "pathlen - length of random walk\n"
               "samples - number of samples to take\n"
               "seed - RNG seed (optional)\n",
               argv[0]);
	exit(1);
  }

  int nruns = 4;
  int cf = strtol(argv[2], NULL, 0);
  int ct = strtol(argv[3], NULL, 0);
  int pathlen = strtol(argv[4], NULL, 0);
  int samples = strtol(argv[5], NULL, 0);
  unsigned int seed = 0;
  if (argc == 7) seed = strtol(argv[6], NULL, 0);
  if (seed == 0) seed = goodseed();
  printf("seed: %d\n", seed);
  srand(seed);
  FILE *gfile = fopen(argv[1], "r");
  assert (gfile != NULL);

  graph *g = graph_read(gfile);
  double *vals = genvals(g);
  fclose(gfile);

  for (int c = cf; c <= ct; ++c) {
    for (int i = 0; i < nruns; ++i) {
      uint64_t before = get_ns();
      double res = rwalk(g, vals, pathlen, c, samples);
      uint64_t after = get_ns();
      uint64_t elapsed = after - before;
      double rate = elapsed;
      rate /= samples*pathlen;
      printf("answer: %f\n", res);
      printf("%d: %lu ns -> %f ns/edge \n", c, elapsed, rate);
    }
  }

  graph_free(g);
  free(vals);
  return 0;
}
