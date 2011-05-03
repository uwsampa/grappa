#define _GNU_SOURCE
#include <assert.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <sys/timeb.h>
#include <string.h>
#include <inttypes.h>

#define REPS 3

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

  return seed;
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

typedef struct node {
  struct node *next;
  uint64_t id;
  char padding[48];
} node;

typedef struct sync {
  int32_t val;
  char padding[56];
} sync;

void pointerify(node *arr, uint64_t len, int threads,
                node **starts, int cores) {
  assert(len <= RAND_MAX);
  assert(len <= 2147483648);
  uint64_t permb = get_ns();
  int32_t n = len;
  int32_t *perm = calloc(sizeof(int32_t), n);
  assert(perm != NULL);

  #define CHUNKS 128
  sync locperms[CHUNKS];
  sync locends[CHUNKS];
  assert (n % CHUNKS == 0);
  int chunksize = n/CHUNKS;
  for (int i = 0; i < CHUNKS; ++i) {
    locperms[i].val = chunksize*i;
    locends[i].val = chunksize*(i+1);
  }
  int started = 0;
  uint64_t placeb;
  //  #pragma omp parallel num_threads(1)
  {
    cpu_set_t cset;
    CPU_ZERO(&cset);
    int mycore = omp_get_thread_num();
    //    mycore = mycore + (mycore%2)*11;
    CPU_SET(mycore, &cset);
    sched_setaffinity(0, sizeof(cpu_set_t), &cset);

    struct random_data rdata;
    memset(&rdata, 0, sizeof(rdata));

    char state[128];
    int seed = goodseed();
    //    #pragma omp critical
    {
      printf("seeding thread %d with %d\n", omp_get_thread_num(), seed);
    }
    initstate_r(seed, state, 128, &rdata);
    //    #pragma omp for
    for (int i = 0; i < n; ++i) {
      int choice;
      int p,e;
      do {
        choice = rand_range(0, CHUNKS, &rdata);
        p = __sync_fetch_and_add(&(locperms[choice].val), 1);
        e = locends[choice].val;
      } while(p >= e);
      perm[p] = i;
    }
    //    #pragma omp for
    for (int c = 0; c < CHUNKS; ++c) {
      int32_t *local = perm+chunksize*c;
      for (int i = chunksize - 1; i > 0; --i) {
        int j = rand_range(0,i+1, &rdata);
        int32_t temp = local[i];
        local[i] = local[j];
        local[j] = temp;
      }
    }
    //    #pragma omp single
    {
      uint64_t perma = get_ns();
      printf("perm: %f\n", ((double)perma - permb)/n);
      placeb = get_ns();
    }
    int32_t stride = n / threads;
    //    #pragma omp for
    for (int32_t i = 0; i < n; ++i) {
      node *curr = &arr[perm[i]];
      if (i % stride == 0) {
        starts[i/stride] = curr;
        #pragma omp critical
        {
          started++;
        }
      }
      if ((i+1) % stride == 0) {
        curr->next = NULL;
      } else {
        curr->next = &arr[perm[i+1]];
      }
    }
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
  free(perm);
  
  uint64_t placea = get_ns();
  printf("place: %f\n", ((double)placea - placeb)/n);
}

void chase(node *start) {
  node *realstart = start;
  for (int i = 0; i < REPS; ++i) {
  node *n;
  start = realstart;
  while((n = start->next) != 0) {
    start = n;
  }
  }
}

void threaded_chase(node **starts) {
  for (int i = 0; i < REPS; ++i) {
  #include "pchase_unrolled.cunroll"
    0;
  }
  return;
}


uint64_t chase_test(int ncores, long long chases, int threads,
                    node *arr, double *bal) {
  int realcores = ncores;
  int threaded = 1;
  if (ncores < 0) {
    threaded = 0;
    ncores = -ncores;
  }
    uint64_t len = chases;
  // len *= threads;
  assert(len % threads == 0);
  //  assert(sizeof(node) == sizeof(node *));
  assert(sizeof(node) == 64);
  node *starts[threads];
  pointerify(arr, len, threads, starts, ncores);
  uint64_t times[ncores];
  int use_core[24];
  for (int i = 0; i < 24; ++i) use_core[i] = 0;
  for (int c = 0; c < ncores; ++c) {
    int mycore;
    #ifdef HYPER
      mycore = c + (c%2)*11;
    #else
    #ifdef NOHYPER
      mycore = 2*c;
    #else
    #error define HYPER or NOHYPER.
    #endif
    #endif
    use_core[mycore] = 1;
  }
#define DEADTHR 1
  #pragma omp parallel for num_threads(ncores+DEADTHR)
  for (int d = 0; d < ncores+DEADTHR; ++d) {
    int c = d-DEADTHR;
    if (d < DEADTHR) {
      cpu_set_t cset;
      CPU_ZERO(&cset);
      for (int i = 1; i <24; i+=2) {
        CPU_SET(i, &cset);
      }
      sched_setaffinity(0, sizeof(cpu_set_t), &cset);
    } else {
    cpu_set_t cset;
    CPU_ZERO(&cset);
    int mycore;
    // very specific to n01, socket 0
    #ifdef HYPER
    mycore = c + (c%2)*11;
    #else
      #ifdef NOHYPER
      mycore = 2*c; 
      #else
      #error define HYPER or NOHYPER.
      #endif
    #endif
    #pragma omp critical
    {
      printf("%d %d: core %d\n", c, omp_get_thread_num(), mycore);
    }
    CPU_SET(mycore, &cset);

    sched_setaffinity(0, sizeof(cpu_set_t), &cset);
    }
    //    if (c >= 0) chase(starts[c]);
    #pragma omp barrier
    if (c >= 0) {
    uint64_t loc_b, loc_a;
    if (threaded) {
      assert (threads == NTHR * ncores);
      loc_b = get_ns();
      threaded_chase(starts + c*NTHR);
      loc_a = get_ns();
    } else {
      loc_b = get_ns();
      chase(starts[c]);
      loc_a = get_ns();
    }
    uint64_t loctime;
    loctime = loc_a - loc_b;
    times[c] = loctime;
    }
  }
  uint64_t mint = -1, maxt = 0;
  for (int i = 0; i < ncores; ++i) {
    printf("time %d,%d,%d: %lu\n", realcores, NTHR, i, times[i]);
    if (mint > times[i]) mint = times[i];
    if (maxt < times[i]) maxt = times[i];
  }
  double balance = maxt;
  balance /= mint;
  printf("total: %lu\n", maxt);
  printf("balance: %f\n", balance);
  *bal = balance;
  //  free(arr);
  //  free(starts);
  return (maxt);
}


int main(int argc, char *argv[]) {
  srand(1);
  if (argc != 4) {
    printf("Usage: %s <cores> <chases> <ignored>\n", argv[0]);
    exit(1);
  }
  // hopefully chases is multiple of 110880 (lcm of 1..12 + 32)
  // so any number of cores + threads we're likely to choose will work
  // 554400000 is about 4 gigabytes
  long long basechases = strtoll(argv[2], NULL, 0);
  #define MTHR 16384
  #if MTHR < NTHR
  #error too many threads
  #endif
  printf("arr: %llu\n",basechases+MTHR*12);
  node *arr = calloc(sizeof(node), basechases + MTHR*12);
  assert(arr != NULL);
  int ncores = strtol(argv[1], NULL, 0);
  assert (-6 <= ncores);
  assert (ncores <= 6);
  #ifdef HYPER
  ncores *= 2;
  #endif
  long long chases = basechases;
  int threads;
  if (ncores < 0) {
    threads = -ncores;
  } else {
    threads = NTHR*ncores;
  }
  assert(threads <= MTHR*12);
  chases += (threads-(chases %threads))% threads;
  for (int i = 0; i < 3; ++i) {
    double bal;
    uint64_t elapsed = 0;
    elapsed += chase_test(ncores, chases, threads, arr, &bal);
    double avg = chases;
    avg *= 1000*REPS;
    // avg *= abs(ncores);
    // avg *= ncores > 0 ? NTHR : 1;
    printf("%f\n", avg);
    avg /= elapsed;
    char *ht;
    #ifdef HYPER
    ht = "h";
    #else
    #ifdef NOHYPER
    ht = "nh";
    #else
    #error Define HYPER or NOHYPER
    #endif
    #endif
    char *pf = PREFETCH ? "p" : "np";
    printf("%fM chases/s (%d threads, %d cores) %f ,%s,%s\n",
           avg, NTHR, ncores,bal,ht,pf);
  }
  free(arr);
}
