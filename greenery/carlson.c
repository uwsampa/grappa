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
  uint64_t id; /* id 0... of thread relative to others assigned same core */
  int64_t upra; /* local updates per remote read */
  int64_t ls; /* log of size of per thread local memory in words */
  int64_t s;  /* size of per thread local memory in words */
  int64_t rs; /* log of size of remote memory allocated in words */
  int64_t * loc; /* base address of local memory block for this core */
  int64_t * rem; /* base address of remote memory */
  uint64_t use_local_prefetch_and_switch; /* boolean to activate latency tolerance for local updates */
  uint64_t local_random_access; /* update local data in random pattern; otherwise sequential */
  uint64_t issue_remote_reference; /* issue prefetch and switch for remote reference */
  uint64_t use_remote_reference; /* use result of prefetch and switch for remote reference */
} loop_arg;

#define RAND(s) (6364136223846793005 * s + 1442695040888963407)
#define ITERS 1000000
static int64_t loop_result;
/* repeatedly reference randomly a "remote" word and
** then update a number of "local" words */
void loopx(thread * me, void *arg) {
  loop_arg * la = (loop_arg *) arg;
  uint64_t rand = RAND(((uint64_t)me));
  /* start of this coroutine's space: */
  //int64_t * local = &la->loc[(1<<(la->ls))*la->id];
  uint64_t * local = (uint64_t*) &la->loc[la->s * la->id];

  for (uint64_t i = 0; i < ITERS; i++) {
    uint64_t index = (rand >> 32) & ((1<<(la->rs))-1);
    if (la->issue_remote_reference)
      prefetch_and_switch(me, &la->rem[index], 0);
    
    //if (la->use_remote_reference)
	rand = RAND(rand + la->rem[index]);
	//else
	//rand = RAND(rand);

    /*     fprintf(stderr, "\n(%ld %ld %ld) ", i, index, rand); */
	for (uint64_t j = 0; j < (uint64_t)la->upra; j++) {
      //int64_t index = (rand >> 32) & ((1<<(la->ls))-1);
      uint64_t index = (rand >> 32) % (la->s);

      //      if (0) //la->use_local_prefetch_and_switch)
      //	prefetch_and_switch(me, &local[index], 1);
     
     if (la->local_random_access)
       rand = RAND(rand + (local[index]+=1));
     else
       local[j % (la->s)]+=1;
       //local[j & ((1<<(la->ls))-1)]+=1;

      /* fprintf(stderr, "(%ld %ld %ld) ", j, index, rand); */
    }
  }
  loop_result =rand; /* use result to avoid dead code elimination */
}

void loop(thread * me, void *arg) {
  loop_arg * la = (loop_arg *) arg;
  int64_t rand = RAND(((int64_t)me));
  /* start of this coroutine's space: */
  //int64_t * local = &la->loc[(1<<(la->ls))*la->id];
  int64_t * local = &la->loc[la->s * la->id];

  for (int64_t i = 0; i < ITERS; i++) {
    //int64_t index = (rand >> 32) & ((1<<(la->rs))-1);
    int64_t index = (rand >> 32) & ((1<<(la->rs))-1);
 
    if (la->issue_remote_reference) 
      prefetch_and_switch(me, &la->rem[index], 0);

    if (la->use_remote_reference)
      rand = RAND(rand + la->rem[index]);
    else
      rand = RAND(rand);

    /*     fprintf(stderr, "\n(%ld %ld %ld) ", i, index, rand); */
    if (la->local_random_access) {
      for (int64_t j = 0; j < la->upra; j++) {
	int64_t index = (rand >> 32) & ((1<<(la->ls))-1);
	//int64_t index = (rand >> 32) & ((1<<(la->ls + 1))-1);
	//if (index > la->s) index -= la->s;
	
	/*      prefetch_and_switch(me, &local[index], 1);*/
	// random
	rand = RAND(rand + (local[index]+=1));
	
	/* local[j & ((1<<(la->ls))-1)]+=1; */
	/* fprintf(stderr, "(%ld %ld %ld) ", j, index, rand); */
      }
    } else {
      int64_t index = (rand >> 32) & ((1<<(la->ls))-1);
      //int64_t index = (rand >> 32) & ((1<<(la->ls + 1))-1);
      for (int64_t j = 0; j < la->upra; j++) {
	if (index > la->s) index -= la->s;
	local[index]+=1;
	index++;
      }
    }
  }
  loop_result =rand; /* use result to avoid dead code elimination */
}


/* void loopz(thread * me, void *arg) { */
/*   loop_arg * la = (loop_arg *) arg; */
/*   int64_t rand = RAND(((int64_t)me)); */
/*   /\* start of this coroutine's space: *\/ */
/*   int64_t * local = &la->loc[(1<<(la->ls))*la->id]; */

/*   for (int64_t i = 0; i < ITERS; i++) { */
/*     int64_t index = (rand >> 32) & ((1<<(la->rs))-1); */
/*     prefetch_and_switch(me, &la->rem[index], 0); */
/*     rand = RAND(rand + la->rem[index]); */
/*     /\*     fprintf(stderr, "\n(%ld %ld %ld) ", i, index, rand); *\/ */
/*     for (int64_t j = 0; j < la->upra; j++) { */
/*       int64_t index = (rand >> 32) & ((1<<(la->ls))-1); */
/*       /\*      prefetch_and_switch(me, &local[index], 1);*\/ */
/*       rand = RAND(rand + (local[index]+=1)); */
/*       /\* local[j & ((1<<(la->ls))-1)]+=1; *\/ */
/*       /\* fprintf(stderr, "(%ld %ld %ld) ", j, index, rand); *\/ */
/*     } */
/*   } */
/*   loop_result =rand; /\* use result to avoid dead code elimination *\/ */
/* } */



int main(int argc, char *argv[]) {
  assert(argc == 9);

  int64_t ncores = atoi(argv[1]);
  //assert(ncores == 1);
  int64_t threads_per_core = atoi(argv[2]);
  int64_t local_updates_per_remote_access = atoi(argv[3]);
  int64_t tflog = atoi(argv[4]);

  int64_t uselocalpns = atoi(argv[5]);
  int64_t lra = atoi(argv[6]);
  int64_t irr = atoi(argv[7]);
  int64_t urr = atoi(argv[8]);

  //int64_t lflog = tflog; // / threads_per_core;
  int64_t total_thread_footprint = (1 << tflog) * threads_per_core * ncores;

  int64_t thread_footprint = 1<<tflog;
  //int64_t thread_footprint = total_thread_footprint / threads_per_core / ncores;


  /* we should change to allocate on processor local to particular core */
  int64_t * local = malloc(thread_footprint*ncores*threads_per_core*sizeof(int64_t));
  /* probably not necessary to zero out memory, but it doesn't cost much */
  bzero(local, thread_footprint*ncores*threads_per_core*sizeof(int64_t));

  int64_t * remote, remote_size = ((int64_t)1) <<27; /* < #phys 64bit words */
  while (!(remote = malloc(remote_size*sizeof(int64_t)))) remote_size >>= 1;
  bzero(remote, remote_size*sizeof(int64_t));

  int64_t rslog = 0;
  while((remote_size >>= 1) >= 1) rslog ++;
  remote_size = ((int64_t) 1) << rslog;
  
  fprintf(stderr,
	  "ncores: %ld; threads per core: %ld; updates per remote: %ld;"
	  " total bytes: %ld local bytes: %ld (%ld); remote bytes: %ld (%ld) total refs: %ld "
	  " localpns: %ld lra: %ld irr: %ld urr: %ld ",
	  ncores, threads_per_core,  local_updates_per_remote_access,
	  total_thread_footprint*sizeof(int64_t), thread_footprint*sizeof(int64_t), tflog, remote_size*sizeof(int64_t), rslog, 
	  ncores*threads_per_core*ITERS*(local_updates_per_remote_access+1),
	  uselocalpns, lra, irr, urr
	  );

  struct timeval * begins = malloc( ncores * sizeof(struct timeval) );
  struct timeval * ends = malloc( ncores * sizeof(struct timeval) );
  struct timeval otv1,otv2;
  gettimeofday(&otv1,0);
  #pragma omp parallel for
  for (int64_t c = 0; c < ncores; c++) {
    loop_arg * la = (loop_arg *) malloc(sizeof(loop_arg)*threads_per_core);

    /* cpu_set_t set; */
    /* CPU_ZERO(&set); */
    /* CPU_SET(c*2 , &set); */
    /* sched_setaffinity(0, sizeof(cpu_set_t), &set); */
    
    /* nodemask_t alloc_mask; */
    /* nodemask_zero(&alloc_mask); */
    /* nodemask_set_compat(&alloc_mask, 0x0); */
    /* numa_set_membind_compat(&alloc_mask); */

  
    thread *master;
    scheduler *sched;
    thread *thread;
    #pragma omp critical (crit_init)
    {
      master = thread_init();
      sched = create_scheduler(master);
    }

    int64_t tpclog = 0;
    switch (threads_per_core) {
    case 1: tpclog = 0; break;
    case 2: tpclog = 1; break;
    case 4: tpclog = 2; break;
    case 8: tpclog = 3; break;
    case 16: tpclog = 4; break;
    case 32: tpclog = 5; break;
    case 64: tpclog = 6; break;
    case 128: tpclog = 7; break;
    case 256: tpclog = 8; break;
    case 512: tpclog = 9; break;
    case 1024: tpclog = 10; break;
    default: tpclog = 11; break;
    }

      for (int64_t i = 0; i < threads_per_core; ++i) {
      la[i].id = i;
      la[i].upra = local_updates_per_remote_access;
      la[i].s = thread_footprint;
      la[i].ls = tflog - tpclog;
      la[i].rs = rslog;
      la[i].loc = local;
      la[i].rem = remote;
      la[i].use_local_prefetch_and_switch = uselocalpns;
      la[i].local_random_access = lra;
      la[i].issue_remote_reference = irr ? -1 : 0;
      la[i].use_remote_reference = urr ? -1 : 0;
      
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
      #pragma omp critical (crit_create)
      thread = thread_spawn(master, sched, loop, (void *) &la[i]);
  }



  struct timeval tv1,tv2;
  gettimeofday(&tv1,0);
  run_all(sched);
  gettimeofday(&tv2,0);
  begins[c] = tv1;
  ends[c] = tv2;
  //double elapsed_sec = ((tv2.tv_sec-tv1.tv_sec)*1000000.0 + (tv2.tv_usec-tv1.tv_usec))/1000000.0;

  //fprintf(stderr, "ref/s: %g ", (ncores*threads_per_core*ITERS*(local_updates_per_remote_access+1))/elapsed_sec);
  //fprintf(stderr, "latency: %g sec\n", ((elapsed_sec)/(ncores*threads_per_core*ITERS*(local_updates_per_remote_access+1))));
#endif 

  destroy_scheduler(sched);
  destroy_thread(master);
}
  gettimeofday(&otv2,0);


otv1 = begins[0];
otv2 = ends[0];
for(int c = 1; c < ncores; c++) {
  if ( timercmp(&otv1, &begins[c], >) )
    otv1 = begins[c];
  if ( timercmp(&otv2, &ends[c], <) )
    otv2 = ends[c];
 }

double elapsed_sec = ((otv2.tv_sec-otv1.tv_sec)*1000000.0 + (otv2.tv_usec-otv1.tv_usec))/1000000.0;

fprintf(stderr, "ref/s: %g ", (ncores*threads_per_core*ITERS*(local_updates_per_remote_access+1))/elapsed_sec);
fprintf(stderr, "latency: %g sec\n", ((elapsed_sec)/(ncores*threads_per_core*ITERS*(local_updates_per_remote_access+1))));

  return 0;
}

