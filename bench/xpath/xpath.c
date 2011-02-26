#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <sys/timeb.h>
#include "graph.h"
#include "greenery/thread.h"

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
static int rand_range(int i, int j) {
  int spread = j - i;
  int limit = RAND_MAX - RAND_MAX % spread;
  int rnd;
  do {
    rnd = rand();
  } while (rnd >= limit);
  rnd %= spread;
  return (rnd + i);
}

struct xpath_args {
  graph *g;
  int *colors;
  int *path;
  int len;
  int t;
  int threads;
  int *loc_c;
  int *edges;
};

void xpath_greenery(thread *me, void *arg) {
  struct xpath_args *args = arg;
  graph *g = args->g;
  int *colors = args->colors;
  int *path = args->path;
  int len = args->len;
  int edges = 0;
  int count = 0;
  // explicit DFS stack.
  uint64_t candidate[len];
  // on the i-th stack level, how far we are in to the edge list of that vertex.
  // we could lookup limit again each time, but that's likely slow - should
  // maybe benchmark this?
  uint64_t *examine[len];
  uint64_t *limit[len];
  uint64_t *temp_vertex;
  int *temp_color;
  uint64_t chunk_size = (g->v+args->threads - 1) / args->threads;
  uint64_t root = args->t*chunk_size;
  uint64_t stop = root + chunk_size;
  stop = stop > g->v ? g->v : stop;
  int d, i, temp;
  for (; root < stop; ++root) {
    if (colors[root] != path[0]) continue;
    d = 0;
    // invariant: only store to candidate[k] if
    // * candidate[0..k] (inclusive) is a valid path (distinct v)
    // * matches colors
    candidate[0] = root;
    temp_vertex = &g->row_ptr[root];
    prefetch_and_switch(me, temp_vertex, 0);
    examine[0] = &g->edges[*temp_vertex++];
    limit[0] = &g->edges[*temp_vertex];
    while (d >= 0) {
      if (d == len - 1) {
        // yes!
        /*
        #pragma omp critical
        {
          printf("path:");
          for (int i = 0; i < len; ++i) {
            printf(" %" PRIu64 "", candidate[i]);
          }
          printf("\n");
        }
        */
        count++;
        d--;
        continue;
      }
      if (examine[d] == limit[d]) {
        // end of the line, nowhere to go
        d--;
        continue;
      }
      temp_vertex = examine[d]++;
      prefetch_and_switch(me, temp_vertex, 0);
      uint64_t next = *temp_vertex;
      temp = 1;
      // c
      for (i = 0; i <= d && temp; ++i) {
        if (candidate[i] == next) temp = 0;
      }
      if (temp) {
        temp_color = &colors[next];
        prefetch_and_switch(me, temp_color, 0);
        if (*temp_color == path[d+1]) {
          edges++;
          d++;
          candidate[d] = next;
          temp_vertex = &g->row_ptr[next];
          prefetch_and_switch(me, temp_vertex, 0);
          examine[d] = &g->edges[*temp_vertex++];
          limit[d] = &g->edges[*temp_vertex];
        }
      }
    }
  }
  *args->loc_c += count;
  *args->edges += edges;

  thread_exit(me, NULL);
}

int xpath_bare(graph *g, int *colors, int *path, int len,
               int *global_count, int c, int ncores, int nthreads) {
  int edges = 0;
  int count = 0;
  // explicit DFS stack.
  uint64_t candidate[len];
  // on the i-th stack level, how far we are in to the edge list of that vertex.
  // we could lookup limit again each time, but that's likely slow - should
  // maybe benchmark this?
  uint64_t *examine[len];
  uint64_t *limit[len];
  uint64_t chunk_size = (g->v+ncores - 1) / ncores;
  uint64_t root = c*chunk_size;
  uint64_t stop = root + chunk_size;
  stop = stop > g->v ? g->v : stop;
  int d, i, temp;
  for (; root < stop; ++root) {
    if (colors[root] != path[0]) continue;
    d = 0;
    // invariant: only store to candidate[k] if
    // * candidate[0..k] (inclusive) is a valid path (distinct v)
    // * matches colors
    candidate[0] = root;
    examine[0] = &g->edges[g->row_ptr[root]];
    limit[0] = &g->edges[g->row_ptr[root+1]];
    while (d >= 0) {
      if (d == len - 1) {
        // yes!
        /*
        #pragma omp critical
        {
          printf("path:");
          for (int i = 0; i < len; ++i) {
            printf(" %" PRIu64 "", candidate[i]);
          }
          printf("\n");
        }
        */
        count++;
        d--;
        continue;
      }
      if (examine[d] == limit[d]) {
        // end of the line, nowhere to go
        d--;
        continue;
      }
      uint64_t next = *examine[d]++;
      temp = 1;
      // c
      for (i = 0; i <= d && temp; ++i) {
        if (candidate[i] == next) temp = 0;
      }
      if (temp && colors[next] == path[d+1]) {
        edges++;
        d++;
        candidate[d] = next;
        examine[d] = &g->edges[g->row_ptr[next]];
        limit[d] = &g->edges[g->row_ptr[next+1]];
      }
    }
  }
  __sync_fetch_and_add(global_count, count);
  return edges;
}

#define NEXT(VAR) ((VAR+1) % active)

int xpath_lightweight(graph *g, int *colors, int *path, int len,
                      int *global_count, int c, int nthreads, int ncores) {
  int edges = 0;
  int count = 0;
  // explicit DFS stack.
  uint64_t candidates[nthreads][len];
  // on the i-th stack level, how far we are in to the edge list of that vertex.
  // we could lookup limit again each time, but that's likely slow - should
  // maybe benchmark this?
  uint64_t *examines[nthreads][len];
  uint64_t *limits[nthreads][len];
  uint64_t chunk_size = (g->v+ncores*nthreads - 1) / (ncores*nthreads);
  uint64_t roots[nthreads];
  uint64_t stops[nthreads];
  int depths[nthreads];
  int *cptrs[nthreads];
  void *pcs[nthreads];
  // array indices and the like
  int i, temp;
  // active thread
  int t;
  int active = nthreads;
  for (t = 0; t < nthreads; ++t) {
    pcs[t] = &&loop_start;
    roots[t] = c*nthreads+t;
    roots[t] *= chunk_size;
    stops[t] = roots[t]+chunk_size;
    stops[t] = stops[t] > g->v ? g->v : stops[t];
    /*
    #pragma omp critical
    {
      printf("%d/%d: [%" PRIu64 ", %" PRIu64 ")\n", c, t, roots[t], stops[t]);
    }*/
  }
  t = 0;
  goto *pcs[t];
  loop_start:
  for (; roots[t] < stops[t]; ++roots[t]) {
    if (colors[roots[t]] != path[0]) continue;
    depths[t] = 0;
    // invariant: only store to candidate[k] if
    // * candidate[0..k] (inclusive) is a valid path (distinct v)
    // * matches colors
    candidates[t][0] = roots[t];
    limits[t][0] = &g->row_ptr[roots[t]];
    __builtin_prefetch(limits[t][0], 0, 0);
    pcs[t] = &&loop_root;
    t = NEXT(t);
    goto *pcs[t];
    loop_root:
    examines[t][0] = &g->edges[*limits[t][0]++];
    limits[t][0] = &g->edges[*limits[t][0]];
    while (depths[t] >= 0) {
      if (depths[t] == len - 1) {
        // yes!
        /*
        #pragma omp critical
        {
          printf("path:");
          for (int i = 0; i < len; ++i) {
            printf(" %" PRIu64 "", candidate[i]);
          }
          printf("\n");
        }
        */
        count++;
        depths[t]--;
        continue;
      }
      if (examines[t][depths[t]] == limits[t][depths[t]]) {
        // end of the line, nowhere to go
        depths[t]--;
        continue;
      }
      __builtin_prefetch(examines[t][depths[t]], 0, 0);
      pcs[t] = &&loop_next;
      t = NEXT(t);
      goto *pcs[t];
      loop_next:
      // cache <next> here for the context switch - breaks invariant,
      // but simplest option
      candidates[t][depths[t]+1] = *examines[t][depths[t]]++;
      temp = 1;
      // c
      for (i = 0; i <= depths[t] && temp; ++i) {
        if (candidates[t][i] == candidates[t][depths[t]+1]) temp = 0;
      }
      if (temp) {
        cptrs[t] = &colors[candidates[t][depths[t]+1]];
        __builtin_prefetch(cptrs[t], 0, 0);
        pcs[t] = &&loop_color;
        t = NEXT(t);
        goto *pcs[t];
        loop_color:
        if (*cptrs[t] == path[depths[t]+1]) {
          edges++;
          depths[t]++;
          limits[t][depths[t]] = &g->row_ptr[candidates[t][depths[t]]];
          __builtin_prefetch(limits[t][depths[t]], 0, 0);
          pcs[t] = &&loop_dive;
          t = NEXT(t);
          goto *pcs[t];
          loop_dive:
          examines[t][depths[t]] = &g->edges[*limits[t][depths[t]]++];
          limits[t][depths[t]] = &g->edges[*limits[t][depths[t]]];
        }
      }
    }
  }
  if (active > 1) {
    // nuke this thread by overwriting with the last one in the array.
    //
    active--;
    roots[t] = roots[active];
    stops[t] = stops[active];
    depths[t] = depths[active];
    cptrs[t] = cptrs[active];
    pcs[t] = pcs[active];
    for (int i = 0; i < len; ++i) {
      candidates[t][i] = candidates[active][i];
      examines[t][i] = examines[active][i];
      limits[t][i] = limits[active][i];
    }
    t = NEXT(t);
    goto *pcs[t];
  }
  __sync_fetch_and_add(global_count, count);
  return edges;
}

int xpath(graph *g, int *colors, int *path, int len,
          int *count, int ncores, int nthreads) {
  int edges = 0;
  
  #pragma omp parallel for num_threads(ncores)
  for (int c = 0; c < ncores; ++c) {
    uint64_t before = get_ns();
    int loc_e = 0;
    if (nthreads == 0) {
      loc_e = xpath_bare(g, colors, path, len, count, c, ncores, nthreads);
    } else if (nthreads < 0) {
      loc_e = xpath_lightweight(g, colors, path, len,
                                count, c, -nthreads, ncores);
    } else {
      thread *master = thread_init();
      scheduler *sched = create_scheduler(master);
      thread *threads[nthreads];
      struct xpath_args args[nthreads];
      int loc_c = 0;
      for (int i = 0; i < nthreads; ++i) {
        args[i].g = g;
        args[i].colors = colors;
        args[i].path = path;
        args[i].len = len;
        args[i].t = i+c*nthreads;
        args[i].threads = nthreads*ncores;
        args[i].loc_c = &loc_c;
        args[i].edges = &loc_e;
        threads[i] = thread_spawn(master, sched, xpath_greenery, (void *)&args[i]);
      }
      
      run_all(sched);
      
      __sync_fetch_and_add(count, loc_c);
      for (int i = 0; i < nthreads; ++i) {
        destroy_thread(threads[i]);
      }
      destroy_scheduler(sched);
      destroy_thread(master);
    }
    uint64_t after = get_ns();
    #pragma omp critical
    {
      edges += loc_e;
    }
  }
  return edges;
}


// does a brute force search for paths - very slow O(V^len), hard to get wrong.
int xpath_brute(graph *g, int *colors, int *path, int len) {
  uint64_t candidate[len];
  for (int i = 0; i < len; ++i) {
    candidate[i] = 0;
  }
  int count = 0;
  int npaths = 0;
  while (1) {
    if (0 == npaths++ % 1000000) {
      printf("Checking:");
      for (int i = 0; i < len; ++i) {
        printf(" %" PRIu64 "", candidate[i]);
      }
      printf("\n");
    }
    // check this path
    // I could stop the loop early at many points here, but that's not a huge
    // savings.
    int pcount = 1;

    for (int i = 0; i < len; ++i) {
      for (int j = 0; j < i; ++j) {
        if (candidate[i] == candidate[j]) pcount = 0;
      }
    }
    if (colors[candidate[0]] != path[0]) pcount = 0;
    
    for (int i = 1; i < len; ++i) {
      uint64_t from = candidate[i-1], to = candidate[i];
      pcount *= graph_edgecount(g, from, to);
      if (colors[to] != path[i]) pcount = 0;
    }

    count += pcount;
    if (pcount > 0) {
      printf("%d paths:", pcount);
      for (int i = 0; i < len; ++i) {
        printf(" %" PRIu64 "", candidate[i]);
      }
      printf("\n");
    }
    int stop = 1;
    for (int i = 0; i < len; ++i) {
      if (candidate[i] != (g->v - 1)) {
        stop = 0;
      }
    }
    if (stop) break;

    for (int i = len -1; i >= 0; --i) {
      if (candidate[i] == (g->v - 1)) {
        candidate[i] = 0;
      } else {
        candidate[i]++;
        break;
      }
    }
  }
  return count;
}

int main(int argc, char *argv[]) {
  if (argc != 6 && argc != 7) {
	printf("usage: %s file nthreads ncores ncolors path seed\n"
               "file     - input graph file\n"
               "nthreads - number of green threads\n"
               "ncores   - number of hardware threads\n"
               "ncolors - color choices\n"
               "path - length of target path\n"
               "seed - RNG seed (optional)\n",
               argv[0]);
	exit(1);
  }

  int nruns = 4;
  int anthreads = strtol(argv[2], NULL, 0);
  int ancores = strtol(argv[3], NULL, 0);
  int ncolors = strtol(argv[4], NULL, 0);
  int pathlen = strtol(argv[5], NULL, 0);
  unsigned int seed = 0;
  if (argc == 7) seed = strtol(argv[6], NULL, 0);
  if (seed == 0) seed = goodseed();
  printf("seed: %d\n", seed);
  srand(seed);
  FILE *gfile = fopen(argv[1], "r");
  assert (gfile != NULL);

  graph *g = graph_read(gfile);
  fclose(gfile);

  printf("Colors:\n");
  int *colors = calloc(sizeof(int), g->v);
  for (unsigned int i = 0; i < g->v; ++i) {
    colors[i] = rand_range(0, ncolors);
    //    printf("%d: %d\n", i, colors[i]);
  }

  printf("Path:");
  int *path = calloc(sizeof(int), pathlen);
  for (int i = 0; i < pathlen; ++i) {
    // I think by a probabilistic argument this could just be zeros, but w/e
    path[i] = rand_range(0, ncolors);
    printf(" %d", path[i]);
  }
  printf("\n");
  double avg;

  int count;
  //  int actual = xpath_brute(g, colors, path, pathlen);
  //  printf("count: %d\n", actual);
  int ncores, nthreads;
  for (ncores = 1; ncores <=6; ++ncores) {
    for (nthreads = -128; nthreads < 256;) {
      avg = 0;
      for (int i = 0; i < nruns; ++i) {
        count = 0;
        uint64_t before = get_ns();
        // "edges" is a rough count of work - edges "traversed" in the search...
        int edges = xpath(g, colors, path, pathlen, &count, ncores, nthreads);
        printf("Matching paths (explored %d): %d\n", edges, count);
        //assert(count == actual);
        uint64_t after = get_ns();
        uint64_t elapsed = after - before;
        double rate = elapsed;
        rate /= edges;
        avg += rate;
        printf("%" PRIu64 " ns -> %f ns/edge \n", elapsed, rate);
      }
      avg /= nruns;
      printf("AVERAGE %d %d: %f\n", ncores, nthreads, avg);
      if (nthreads == 0) {
        nthreads = 1;
      } else if (nthreads == -1) {
        nthreads = 0;
      } else if (nthreads < 0) {
        nthreads /= 2;
      } else {
        nthreads *= 2;
      }
    }
  }
  graph_free(g);
  free(colors);
  free(path);

  return 0;
}
