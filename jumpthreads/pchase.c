#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <sys/timeb.h>
#include <inttypes.h>

static uint64_t get_ns()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);

  uint64_t ns = 0;
  ns += ts.tv_nsec;
  ns += ts.tv_sec * (1000LL * 1000LL * 1000LL);
  return ns;
}

// returns uniformly chosen integer in [i,j)
static uint64_t rand_range(uint64_t i, uint64_t j) {
  uint64_t spread = j - i;
  uint64_t limit = RAND_MAX - RAND_MAX % spread;
  uint64_t rnd;
  do {
    rnd = rand();
  } while (rnd >= limit);
  rnd %= spread;
  return (rnd + i);
}


uint64_t *genperm(uint64_t n) {
  assert(n <= RAND_MAX);
  uint64_t *perm = calloc(sizeof(uint64_t), n);
  assert(perm != NULL);
  for (uint64_t i = 0; i < n; ++i) {
    perm[i] = i;
  }
  for (uint64_t i = n - 1; i > 0; --i) {
    uint64_t j = rand_range(0, i+1);
    uint64_t temp = perm[i];
    perm[i] = perm[j];
    perm[j] = temp;
  }
  return perm;
}

typedef struct node {
  struct node *next;
} node;

void pointerify(node *arr, uint64_t len, int threads, node **starts) {
  assert(len <= RAND_MAX);
  for (uint64_t i = 0; i < len; ++i) {
    arr[i].next = &arr[i];
  }
  // similar to Fisher-Yates but subtly different; produces one (random) cycle
  // of pointers. pf: exercise for the reader.
  for (uint64_t i = len - 2; i >= 0; --i) {
    uint64_t j = rand_range(i+1,len);
    node *temp = arr[i].next;
    arr[i].next = arr[j].next;
    arr[j].next = temp;
    if (i == 0) break;
  }
  // alas we have to chase the entire thing once to break it into sections
  uint64_t stride = len / threads;
  uint64_t seen = 0;
  int started = 0;
  node *curr = &arr[rand_range(0,len)];
  starts[started++] = curr;
  while (seen < len) {
    node *n = curr->next;
    if (++seen % stride == 0) {
      if (seen < len) {
        starts[started++] = n;
      }
      curr->next = NULL;
    }
    curr = n;
  }
  /*
  for (uint64_t i = 0; i < len; ++i) {
    printf("%p -> %p\n", &arr[i], arr[i].next);
  }

  for (uint64_t i = 0; i < threads; ++i) {
    printf("%d: %p\n", i, starts[i]);
  }
  printf("%d %d\n", started, seen);
  */
  assert(started == threads);
  assert(seen == len);
}

void chase(node *start) {
  node *n;
  while((n = start->next) != 0) {
    start = n;
 } 
}

void threaded_chase(node **starts) {
  #include "pchase_unrolled.cunroll"
  return;
}


uint64_t chase_test(int ncores, long long chases) {
  int threads = NTHR;
  int threaded = 1;
  if (ncores < 0) {
    threaded = 0;
    threads = 1;
    ncores = -ncores;
  }
  threads *= ncores;
  uint64_t len = threads*chases;
  assert(sizeof(node) == sizeof(node *));
  node *arr = calloc(sizeof(node), len);
  node **starts = calloc(sizeof (node*), threads);
  pointerify(arr, len, threads, starts);
  uint64_t before = get_ns();
  #pragma omp parallel for num_threads(ncores)
  for (int c = 0; c < ncores; ++c) {
    if (threaded) {
      threaded_chase(starts + c*NTHR);
    } else {
      chase(starts[c]);
    }
  }
  uint64_t after = get_ns();
  return (after - before);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: %s <cores> <chases per core>\n", argv[0]);
    exit(1);
  }

  int ncores = strtol(argv[1], NULL, 0);
  long long chases = strtoll(argv[2], NULL, 0);
  uint64_t elapsed = chase_test(ncores, chases);
  double avg = chases;
  avg *= 1000;
  avg *= abs(ncores);
  avg *= ncores > 0 ? NTHR : 1;
  printf("%f\n", avg);
  avg /= elapsed;
  printf("%fM chases/s (%d threads)\n", avg, ncores > 0? NTHR : 0);
}
