#define _GNU_SOURCE
#include "thread.h"
#include "stdint.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <strings.h>
#include <sched.h>
#include <numa.h>
/* Bill Carlson asked for measurements of prefetch-and-switch on the abstract
** application of random "remote" references interspersed amongst denser updates
** to "local" memory.
** "Remote" corresponds to "all over the addressible physical memory" and "local"
** to locations that exhibit temporal locality, referencing words in a bounded
** "footprint" of memory.  (I think it might be more
** meaningful to interpret "local" as to references that exhibit also spatial
** locality -- such as occurs naturally with stack.)
** This program is intended to execute that reference pattern across varying
** #cores (currently 1),
** #coroutines per core,
** #local updates per remote reference,
** and #64-bit words in coroutine footprint.
** The idea of multiple threads per core (as in SMT) is not explored;
** nor is the idea of multiple pthreads per core, as the assumption is that would
** just add to the overhead.
*/
typedef struct { /* loop arguments; one struct per core */
  int64_t id; /* id 0... of thread relative to others assigned same core */
  int64_t upra; /* local updates per remote read */
  int64_t ls; /* log of size of per thread local memory in words */
  int64_t rs; /* log of size of remote memory allocated in words */
  int64_t * loc; /* base address of local memory block for this core */
  int64_t * rem; /* base address of remote memory */
} loop_arg;

#define RAND(s) (6364136223846793005 * s + 1442695040888963407)
#define ITERS 1000000
static int64_t loop_result;
/* repeatedly reference randomly a "remote" word and
** then update a number of "local" words */
void loop(thread * me, void *arg) {
  loop_arg * la = (loop_arg *) arg;
  int64_t rand = RAND(((int64_t)me));
  /* start of this coroutine's space: */
  int64_t * local = &la->loc[(1<<(la->ls))*la->id];

  for (int64_t i = 0; i < ITERS; i++) {
    int64_t index = (rand >> 32) & ((1<<(la->rs))-1);
    prefetch_and_switch(me, &la->rem[index]);
    rand = RAND(rand + la->rem[index]);
    /*     fprintf(stderr, "\n(%ld %ld %ld) ", i, index, rand); */
    for (int64_t j = 0; j < la->upra; j++) {
     int64_t index = (rand >> 32) & ((1<<(la->ls))-1);
      /*      prefetch_and_switch(me, &local[index]);*/
      rand = RAND(rand + (local[index]+=1));
      /* local[j & ((1<<(la->ls))-1)]+=1; */
      /* fprintf(stderr, "(%ld %ld %ld) ", j, index, rand); */
    }
  }
  loop_result =rand; /* use result to avoid dead code elimination */
}

int main(int argc, char *argv[]) {
  assert(argc == 5);

  int64_t ncores = atoi(argv[1]);
  assert(ncores == 1);
  int64_t threads_per_core = atoi(argv[2]);
  int64_t local_updates_per_remote_access = atoi(argv[3]);
  int64_t tflog = atoi(argv[4]);

  int64_t thread_footprint = 1<<tflog;

  /* while measuring on just one processor:  */

  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET(0, &set);
  sched_setaffinity(0, sizeof(cpu_set_t), &set);

  nodemask_t alloc_mask;
  nodemask_zero(&alloc_mask);
  nodemask_set_compat(&alloc_mask, 0x1);
  numa_set_membind_compat(&alloc_mask);

  /* we should change to allocate on processor local to particular core */
  int64_t * local = malloc(thread_footprint*threads_per_core*sizeof(int64_t));
  /* probably not necessary to zero out memory, but it doesn't cost much */
  bzero(local, thread_footprint*threads_per_core*sizeof(int64_t));

  int64_t * remote, remote_size = ((int64_t)1) <<27; /* < #phys 64bit words */
  while (!(remote = malloc(remote_size*sizeof(int64_t)))) remote_size >>= 1;
  bzero(remote, remote_size*sizeof(int64_t));

  int64_t rslog = 0;
  while((remote_size >>= 1) >= 1) rslog ++;
  remote_size = ((int64_t) 1) << rslog;
  
  fprintf(stderr,
	  "ncores: %ld; threads per core: %ld; updates per remote: %ld;"
	  " local bytes: %ld (%ld); remote bytes: %ld (%ld) total refs: %ld ",
	  ncores, threads_per_core,  local_updates_per_remote_access,
	  thread_footprint*sizeof(int64_t), tflog, remote_size*sizeof(int64_t), rslog, ncores*threads_per_core*ITERS*(local_updates_per_remote_access+1));

  /*  for (int64_t c = 0; c < ncores; c++) { */
    loop_arg * la = (loop_arg *) malloc(sizeof(loop_arg)*threads_per_core);

    thread *master = thread_init();
    scheduler *sched = create_scheduler(master);

    for (int64_t i = 0; i < threads_per_core; ++i) {
      la[i].id = i;
      la[i].upra = local_updates_per_remote_access;
      la[i].ls = tflog;
      la[i].rs = rslog;
      la[i].loc = local;
      la[i].rem = remote;


#ifdef TEST
  struct timeval tv1,tv2;
  gettimeofday(&tv1,0);
    loop((void*)1,&la[0]);
  gettimeofday(&tv2,0);
  double elapsed_sec = ((tv2.tv_sec-tv1.tv_sec)*1000000.0 + (tv2.tv_usec-tv1.tv_usec))/1000000.0;
fprintf(stderr, "ref/s: %g ", (ncores*threads_per_core*ITERS*(local_updates_per_remote_access+1))/elapsed_sec);
fprintf(stderr, "latency: %g sec\n", ((elapsed_sec)/(ncores*threads_per_core*ITERS*(local_updates_per_remote_access+1))));
    exit(0);
    }
#else
      thread_spawn(master, sched, loop, (void *) &la[i]);
    }
    /*   } */

  struct timeval tv1,tv2;
  gettimeofday(&tv1,0);
  run_all(sched);
  gettimeofday(&tv2,0);
  double elapsed_sec = ((tv2.tv_sec-tv1.tv_sec)*1000000.0 + (tv2.tv_usec-tv1.tv_usec))/1000000.0;

fprintf(stderr, "ref/s: %g ", (ncores*threads_per_core*ITERS*(local_updates_per_remote_access+1))/elapsed_sec);
fprintf(stderr, "latency: %g sec\n", ((elapsed_sec)/(ncores*threads_per_core*ITERS*(local_updates_per_remote_access+1))));
#endif 

  destroy_scheduler(sched);
  destroy_thread(master);

  return 0;
}
