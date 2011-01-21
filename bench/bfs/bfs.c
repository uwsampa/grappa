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

#define CHUNK_SIZE 32

void print_queue(thread *head) {
  while (1) {
    printf("%p", head);
    if (head != NULL) {
      head = head->next;
      printf(" -> ");
    } else {
      break;
    }
  }
  printf("\n");
}

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
    //    printf("%p ours: %d %d\n", ka, ka->in, ka->done);
    while (ka->in < ka->done) {
      uint64_t vertex = ka->from[ka->in++];
      if (vertex == ka->g->v) continue;
      //      printf("%p looking at: %" PRIu64 "\n", ka, vertex);
      uint64_t *addr = &ka->g->row_ptr[vertex];
      prefetch_and_switch(me, addr, 0);
      uint64_t *start = &ka->g->edges[*addr++];
      addr = &ka->g->edges[*addr];
      while (start < addr) {
        prefetch_and_switch(me, start, 0);
        uint64_t next = *start++;
        uint64_t *pa = &ka->parent[next];
        prefetch_and_switch(me, pa, 1);
        while (*pa == ka->g->v) {
          if (!__sync_bool_compare_and_swap(pa, ka->g->v, vertex)) {
            continue;
          }
          if (ka->out >= ka->limit) {
            ka->out = __sync_fetch_and_add(ka->next_chunk, CHUNK_SIZE);
            ka->limit = ka->out + CHUNK_SIZE;
          }
          //          printf("%p Exploring %" PRIu64 "\n", ka, next);
          ka->to[ka->out++] = next;

        }
      }
    }
    thread_block(me, &ka->barrier);
    if (me == ka->leader) {
      //      printf("cleaning %d %d\n", ka->out, ka->limit);
      while (ka->out < ka->limit) {
        ka->to[ka->out++] = ka->g->v;
      }
      //      printf("%p agreed %d\n", ka, *ka->next_chunk);
      #pragma omp barrier
      #pragma omp single
      {
        *ka->level_size = *ka->next_chunk;
        *ka->next_chunk = 0;
        ka->out = ka->limit = 0;
        while(*ka->level_size % ka->ncores > 0) {
          ka->to[(*ka->level_size)++] = ka->g->v;
        }
        //        printf("%p next iteration: %d\n", ka,  *ka->level_size);
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
// this will also break if g is disconnected
void check(graph *g, uint64_t parent[], uint64_t root) {
  for (unsigned int i = 0; i < g->v; ++i) {
    uint64_t p = parent[i];
    //    printf("%d %" PRIu64"\n", i, p);
  }
  for (unsigned int i = 0; i < g->v; ++i) {
    uint64_t p = parent[i];
    //    printf("%d %" PRIu64"\n", i, p);
    if (i == root) {
      assert(p == root);
      continue;
    }
    assert(p < g->v);
    assert (i != p);
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
  if (argc != 5) {
	printf("usage: %s file nthreads ncores root\nfile     - input graph file\nnthreads \
- number of green threads\nroot     - index of root node\n", argv[0]);
	exit(1);
  }

  int nruns = 4;
  int nthreads = strtol(argv[2], NULL, 0);
  int ncores = strtol(argv[3], NULL, 0);
  FILE *gfile = fopen(argv[1], "r");
  assert (gfile != NULL);

  uint64_t root = strtoll(argv[4], NULL, 0);

  graph *g = graph_read(gfile);
  fclose(gfile);
  unsigned int qsize = ncores*CHUNK_SIZE;
  qsize = qsize < g->v? g->v : qsize;
  uint64_t *to_examine = calloc(sizeof(uint64_t), qsize);
  uint64_t *next = calloc(sizeof(uint64_t), qsize);
  uint64_t *parent = calloc(sizeof(uint64_t), g->v);
  double avg = 0;
  for (int i = 0; i < nruns; ++i) {
    for (unsigned int i = 0; i < g->v; ++i) {
      to_examine[i] = next[i] = parent[i] = g->v;
    }
    parent[root] = root;
    uint64_t before = get_ns();
    bfs(g, root, ncores, nthreads,
        to_examine, next, parent);
    uint64_t after = get_ns();

    uint64_t elapsed = after - before;
    double rate = elapsed;
    rate /= g->e;
    avg += rate;
    check(g, parent, root);
    printf("%" PRIu64 " ns -> %f ns/edge \n", elapsed, rate);
  }
  avg /= nruns;
  printf("AVERAGE: %f\n", avg);

  graph_free(g);
  free(to_examine);
  free(next);

  return 0;
}
