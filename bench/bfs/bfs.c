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

// pointers are shared between cores (except leader)
// Nonpointers are shared between green threads.
struct kernel_arg {
  graph *g;
  uint64_t *from, *to, *parent;
  int *level_size;
  int *next_chunk;
  thread *leader;
  thread_barrier barrier;
  int in, done;
  int out, limit;
  int core;
  int ncores;
};

#define CHUNK_SIZE 1024

void bfs_kernel(thread *me, void *arg) {
  struct kernel_arg *ka = arg;
  while (*ka->level_size > 0) {
    thread_block(me, &ka->barrier);
    if (me == ka->leader) {
      // invariant: level_size is a multiple of ncores
      int chunk_size = (*ka->level_size ) / ka->ncores;
      ka->in = chunk_size*ka->core;
      ka->done = ka->in + chunk_size;
    }
    thread_block(me, &ka->barrier);
    while (ka->in < ka->done) {
      uint64_t vertex = ka->from[ka->in++];
      if (vertex == ka->g->v) continue;
      uint64_t *addr = &ka->g->row_ptr[vertex];
      prefetch_and_switch(me, addr, 0);
      uint64_t *start = &ka->g->edges[*addr++];
      addr = &ka->g->edges[*addr];
      while (start < addr) {
        prefetch_and_switch(me, start, 0);
        uint64_t next = *start++;
        if (__sync_bool_compare_and_swap(&ka->parent[next], ka->g->v, vertex)) {
          if (ka->out < ka->limit) {
            ka->to[ka->out++] = next;
          } else {
            ka->out = __sync_add_and_fetch(ka->next_chunk, CHUNK_SIZE);
            ka->limit = ka->out + CHUNK_SIZE;
          }
        }
      }
    }
    thread_block(me, &ka->barrier);
    if (me == ka->leader) {
      while (ka->out < ka->limit) {
        ka->to[ka->out++] = ka->g->v;
      }
#pragma omp single
      {
        *ka->level_size = *ka->next_chunk;
        while(*ka->level_size % ka->ncores > 0) {
          ka->to[(*ka->level_size)++] = ka->g->v;
        }
      }
      uint64_t *temp = ka->to;
      ka->to = ka->from;
      ka->from = temp;
    }
  }
}

void bfs(graph *g, uint64_t root,
         int ncores, int nthreads,
         uint64_t *to_examine, uint64_t *next, uint64_t *parent) {
  to_examine[0] = root;
  int level_size = 1;
  for(; level_size < ncores; ++level_size) {
    to_examine[level_size] = g->v;
  }
  int next_chunk = 0;
#pragma omp parallel for num_threads(ncores)
  for (int c = 0; c < ncores; ++c) {
    thread *master = thread_init();
    scheduler *sched = create_scheduler(master);
    struct kernel_arg ka;
    ka.core = c;
    ka.ncores = ncores;
    ka.g = g;
    ka.from = to_examine;
    ka.to = next;
    ka.parent = parent;
    ka.level_size = &level_size;
    ka.next_chunk = &next_chunk;
    ka.out = ka.limit = 0;
    ka.barrier.sched = sched;
    ka.barrier.size = nthreads;
    ka.barrier.blocked = 0;
    ka.barrier.threads = NULL;
    thread *threads[nthreads];
    for (int i = 0; i < nthreads; ++i) {
      threads[i] = thread_spawn(master, sched, bfs_kernel, &ka);
    }
    ka.leader = threads[0];
    run_all(sched);
    for (int i = 0; i < nthreads; ++i) {
      destroy_thread(threads[i]);
    }
    destroy_scheduler(sched);
    destroy_thread(master);
  }
}


// should probably check shortest paths too?
void check(graph *g, uint64_t parent[], uint64_t root) {
  for (unsigned int i = 0; i < g->v; ++i) {
    uint64_t p = parent[i];
    if (i == root) {
      assert(p == g->v);
      continue;
    }
    assert(p < g->v);
    int found = 0;
    for (uint64_t *e = &g->edges[g->row_ptr[p]],
                  *f = &g->edges[g->row_ptr[p+1]]; e < f; ++e) {
      if (*e == i) {
        found = 1;
        break;
      }
    }
    assert(found);
  }
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
	printf("usage: %s file nthreads ncores\nfile     - input graph file\nnthreads \
- number of green threads\n", argv[0]);
	exit(1);
  }

  int nruns = 4;
  int nthreads = strtol(argv[2], NULL, 0);
  int ncores = strtol(argv[3], NULL, 0);
  FILE *gfile = fopen(argv[1], "r");
  assert (gfile != NULL);

  uint64_t root;
  assert(1 == fscanf(gfile, "%" PRIu64 "\n", &root));

  graph *g = graph_read(gfile);
  fclose(gfile);
  uint64_t *to_examine = calloc(sizeof(uint64_t), g->v);
  uint64_t *next = calloc(sizeof(uint64_t), g->v);
  uint64_t *parent = calloc(sizeof(uint64_t), g->v);
  double avg = 0;
  for (int i = 0; i < nruns; ++i) {
    for (unsigned int i = 0; i < g->v; ++i) {
      to_examine[i] = next[i] = parent[i] = g->v;
    }
    uint64_t before = get_ns();
    bfs(g, root, ncores, nthreads,
        to_examine, next, parent);
    uint64_t after = get_ns();

    uint64_t elapsed = after - before;
    double rate = elapsed;
    rate /= g->v;
    avg += rate;
    check(g, parent, root);
    printf("%" PRIu64 " ns -> %f ns/vertex \n", elapsed, rate);
  }
  avg /= nruns;
  printf("AVERAGE: %f\n", avg);

  graph_free(g);
  free(to_examine);
  free(next);

  return 0;
}
