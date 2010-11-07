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

uint64_t basic_depth(graph *tree, uint64_t root,
                     uint64_t *to_examine, uint64_t *next) {
  unsigned int to_examine_size = 1;
  to_examine[0] = root;
  uint64_t depth = 0;
  while (to_examine_size > 0) {
    depth++;
    unsigned int next_size = 0;
    for (unsigned int i = 0; i < to_examine_size; ++i) {
      uint64_t vertex = to_examine[i];
      uint64_t *addr = &tree->edges[tree->row_ptr[vertex]];
      uint64_t *limit = &tree->edges[tree->row_ptr[vertex+1]];
      while (addr < limit) {
        next[next_size++] = *addr++;
      }
    }

    uint64_t *temp = to_examine;
    to_examine = next;
    next = temp;
    to_examine_size = next_size;
  }
  return depth;
}

struct depth_info {
  graph *tree;
  uint64_t *to_examine;
  uint64_t *next;
  unsigned int to_examine_size; 
  unsigned int next_size;
  unsigned int depth;
  unsigned int i;
  thread_barrier barrier;
  thread *leader;
};

void thread_f(thread *me, void *arg) {
  struct depth_info *di = arg;
  uint64_t *row_ptr = di->tree->row_ptr;
  uint64_t *edges = di->tree->edges;
  uint64_t *addr;
  uint64_t *start;
  //  printf("%d %p %d %d\n", di->depth, me, di->i, di->to_examine_size);
  while (di->to_examine_size > 0) {

    while (di->i < di->to_examine_size) {
      uint64_t vertex = di->to_examine[di->i++];
      //      printf("%p %"PRIu64 "\n", me, vertex);
      addr = &row_ptr[vertex];
      prefetch_and_switch(me, addr);
      start = &edges[*addr++];
      addr = &edges[*addr];
      while (start < addr) {
        prefetch_and_switch(me, start);
        di->next[di->next_size++] = *start++;
      }
      //      printf("%d %p %d %d\n", di->depth, me, di->i, di->to_examine_size);
      //      thread_yield(me);
    }

    thread_block(me, &di->barrier);
    if (me == di->leader) {
      uint64_t *temp = di->to_examine;
      di->to_examine = di->next;
      di->next = temp;
      di->to_examine_size = di->next_size;
      di->depth++;
      di->next_size = 0;
      di->i = 0;
    }
    thread_block(me, &di->barrier);
  }

  thread_exit(me, NULL);
}

uint64_t threaded_depth(graph *tree, uint64_t root,
                        thread *master, scheduler *sched, int nthreads,
                        uint64_t *to_examine, uint64_t *next) {
  struct depth_info di;
  di.tree = tree;
  di.to_examine = to_examine;
  di.next = next;
  di.to_examine_size = 1;
  di.i = 0;
  di.next_size = 0;
  di.to_examine[0] = root;
  di.depth = 0;
  thread *threads[nthreads];

  for (int i = 0; i < nthreads; ++i) {
    threads[i] = thread_spawn(master, sched, thread_f, &di);
  }
  di.leader = threads[0];
  di.barrier.sched = sched;
  di.barrier.size = nthreads;
  di.barrier.blocked = 0;
  di.barrier.threads = NULL;

  run_all(sched);
  for (int i = 0; i < nthreads; ++i) {
    destroy_thread(threads[i]);
  }
  return di.depth;
}

int main(int argc, char *argv[]) {
  assert(argc == 3);
  int nruns = 4;
  int nthreads = strtol(argv[2], NULL, 0);
  FILE *treefile = fopen(argv[1], "r");
  assert (treefile != NULL);

  uint64_t root;
  assert(1 == fscanf(treefile, "%" PRIu64 "\n", &root));

  graph *tree = graph_read(treefile);
  fclose(treefile);
  uint64_t *to_examine = calloc(sizeof(uint64_t), tree->v);
  uint64_t *next = calloc(sizeof(uint64_t), tree->v);
  for (unsigned int i = 0; i < tree->v; ++i) {
    to_examine[i] = next[i] = i;
  }
  printf("BASIC:\n");
  double avg = 0;
  for (int i = 0; i < nruns; ++i) {
    uint64_t before = get_ns();
    uint64_t answer = basic_depth(tree, root, to_examine, next);
    uint64_t after = get_ns();

    uint64_t elapsed = after - before;
    double rate = elapsed;
    rate /= tree->v;
    avg += rate;
    printf("depth is %" PRIu64 "\n", answer);
    printf("%" PRIu64 " ns -> %f ns/vertex \n", elapsed, rate);
  }
  avg /= nruns;
  printf("BASELINE: %f\n", avg);
  printf("THREADED:\n");
  thread *master = thread_init();
  scheduler *sched = create_scheduler(master);
  avg = 0;
  for (int i = 0; i < nruns; ++i) {
    uint64_t before = get_ns();
    uint64_t answer = threaded_depth(tree, root,
                                     master, sched, nthreads,
                                     to_examine, next);
    uint64_t after = get_ns();

    uint64_t elapsed = after - before;
    double rate = elapsed;
    rate /= tree->v;
    avg += rate;
    printf("depth is %" PRIu64 "\n", answer);
    printf("%" PRIu64 " ns -> %f ns/vertex \n", elapsed, rate);
  }
  avg /= nruns;
  printf("AVERAGE: %f\n", avg);
  free(to_examine);
  free(next);

  return 0;
}
