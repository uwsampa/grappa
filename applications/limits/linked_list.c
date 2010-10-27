
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>

#include "node.h"
#include "alloc.h"

#define NPROC 1
#define BITS 23

#define SIZE (1 << BITS)


// walk the list
uint64_t walk( node nodes[], uint64_t count ) {
  node* i = &nodes[0];

  while (count > 0) {
    i = i->next;
    count--;
  }

  return (uint64_t)i; // give the optimizer something to think about
}

// walk the list
uint64_t multiwalk( node* bases[], 
		    uint64_t concurrent_reads, 
		    uint64_t count ) {
  node* i = bases[0];
  node* j = concurrent_reads >=  2 ? bases[1] : NULL;
  node* k = concurrent_reads >=  3 ? bases[2] : NULL;
  node* l = concurrent_reads >=  4 ? bases[3] : NULL;
  node* m = concurrent_reads >=  5 ? bases[4] : NULL;
  node* n = concurrent_reads >=  6 ? bases[5] : NULL;
  node* o = concurrent_reads >=  7 ? bases[6] : NULL;
  node* p = concurrent_reads >=  8 ? bases[7] : NULL;
  node* q = concurrent_reads >=  9 ? bases[8] : NULL;
  node* r = concurrent_reads >= 10 ? bases[9] : NULL;
  node* s = concurrent_reads >= 11 ? bases[10] : NULL;
  node* t = concurrent_reads >= 12 ? bases[11] : NULL;
  
  switch (concurrent_reads) {
  case 1: 
    {
      while (count > 0) {
	i = i->next;
	count--;
      }
    }
    break;

  case 2: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	count--;
      }
    }
    break;

  case 3: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	count--;
      }
    }
    break;

  case 4: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	count--;
      }
    }
    break;


  case 5: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	m = m->next;
	count--;
      }
    }
    break;


  case 6: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	m = m->next;
	n = n->next;
	count--;
      }
    }
    break;


  case 7: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	m = m->next;
	n = n->next;
	o = o->next;
	count--;
      }
    }
    break;

  case 8: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	m = m->next;
	n = n->next;
	o = o->next;
	p = p->next;
	count--;
      }
    }
    break;

  case 9: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	m = m->next;
	n = n->next;
	o = o->next;
	p = p->next;
	q = q->next;
	count--;
      }
    }
    break;

  case 10: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	m = m->next;
	n = n->next;
	o = o->next;
	p = p->next;
	q = q->next;
	r = r->next;
	count--;
      }
    }
    break;

  case 11: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	m = m->next;
	n = n->next;
	o = o->next;
	p = p->next;
	q = q->next;
	r = r->next;
	s = s->next;
	count--;
      }
    }
    break;

  case 12: 
    {
      while (count > 0) {
	i = i->next;
	j = j->next;
	k = k->next;
	l = l->next;
	m = m->next;
	n = n->next;
	o = o->next;
	p = p->next;
	q = q->next;
	r = r->next;
	s = s->next;
	t = t->next;
	count--;
      }
    }
    break;


    }

  return (uint64_t)i
    + (uint64_t)j
    + (uint64_t)k
    + (uint64_t)l
    + (uint64_t)m
    + (uint64_t)n
    + (uint64_t)o
    + (uint64_t)p
    + (uint64_t)q
    + (uint64_t)r
    + (uint64_t)s
    + (uint64_t)t;
}


int main(int argc, char* argv[]) {

  assert(argc == 4);

  uint64_t bits = atoi(argv[1]);
  uint64_t size = (1 << bits);

  uint64_t num_lists_per_thread = atoi(argv[2]);
  uint64_t num_threads = atoi(argv[3]);

  const uint64_t number_of_repetitions = 10;


  // initialize random number generator
  srandom(time(NULL));

  //print( data );


  int n;

  // initialize data structure
  node** bases = allocate_page( size, num_threads, num_lists_per_thread );

  for(n = 0; n < number_of_repetitions; n++) {
    //print( data );

    int thread_num;
    const uint64_t procsize = size / (num_threads * num_lists_per_thread);
    uint64_t* results = malloc( sizeof(uint64_t) * num_threads * num_lists_per_thread); 

    uint64_t* times = malloc( sizeof(uint64_t) * num_threads );

    struct timespec start;
    struct timespec end;
      
    // start timing
    clock_gettime(CLOCK_MONOTONIC, &start);
    //printf("Start time: %d seconds, %d nanoseconds\n", (int) start.tv_sec, (int) start.tv_nsec);

    // do work
    #pragma omp parallel for num_threads(num_threads)
    for(thread_num = 0; thread_num < num_threads; thread_num++) {

      //results[thread_num] = walk( data, thread_num, num_threads, num_lists, procsize );
      results[thread_num] = multiwalk( bases + thread_num * num_lists_per_thread,
				       num_lists_per_thread, procsize );

    }

    // stop timing
    clock_gettime(CLOCK_MONOTONIC, &end);
    //printf("End time: %d seconds, %d nanoseconds\n", (int) end.tv_sec, (int) end.tv_nsec);
    
    uint64_t runtime_ns = 0;
    //times[thread_num] = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec) - ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);
    runtime_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec) - ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);

    // unimportant calculation to ensure calls don't get optimized away
    uint64_t result = 0;
    for(thread_num = 0; thread_num < num_threads; thread_num++) {
      result += results[thread_num];
      //runtime_ns += times[thread_num];
    }

    //uint64_t runtime_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec) - ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);


    const uint64_t totalsize = size; //procsize * num_threads * num_lists_per_thread;
    if (num_threads == 1) assert(totalsize == size);
    double runtime_s = (double)runtime_ns / 1000000000.0;
    const uint64_t total_bytes = totalsize * sizeof(node);
    const uint64_t proc_bytes = procsize * sizeof(node);
    const uint64_t total_requests = totalsize;
    const uint64_t proc_requests = procsize;
    const double   request_rate = (double)total_requests / runtime_s;
    const double   proc_request_rate = (double)proc_requests / runtime_s;
    const double   data_rate = (double)total_bytes / runtime_s;
    const double   proc_data_rate = (double)proc_bytes / runtime_s;
    const double   proc_request_time = (double)runtime_ns / proc_requests;
    const double   proc_request_cpu_cycles = proc_request_time / (1.0 / 2.27);

    printf("{ bits:%lu num_threads:%lu concurrent_reads:%lu total_bytes:%lu proc_bytes:%lu total_requests:%lu proc_requests:%lu request_rate:%f proc_request_rate:%f data_rate:%f proc_data_rate:%f proc_request_time:%f proc_request_cpu_cycles:%f }\n",
	   bits, num_threads, num_lists_per_thread, total_bytes, proc_bytes, total_requests, proc_requests, 
	   request_rate, proc_request_rate, data_rate, proc_data_rate, proc_request_time, proc_request_cpu_cycles);

    printf("(%lu/%lu): %f MB/s, %lu nanoseconds, %lu requests, %f req/s, %f req/s/proc, %f ns/req, %f B/s, %f clocks each\n", 
	   bits, num_threads,
	   (double)total_bytes/runtime_s/1000/1000, 
	   runtime_ns, totalsize, (double)totalsize/((double)runtime_ns/1000000000), 
	   (double)totalsize/((double)runtime_ns/1000000000)/num_threads, 
	   (double)runtime_ns/totalsize, 
	   (double)totalsize*sizeof(node)/((double)runtime_ns/1000000000),
	   ((double)runtime_ns/(totalsize/num_threads)) / (.00000000044052863436 * 1000000000) 
	   );
  }
  //print( data );

  return 0;
}
