
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>

#include "node.h"

#define NPROC 1
#define BITS 23

#define SIZE (1 << BITS)
//#define SIZE ((1 << BITS) + (1 << (BITS-1)))

void print( node nodes[], int size ) {
  printf("[\n");
  uint64_t i = 0;
  for( i = 0; i < size; ++i ) {
    printf("%p: { id: %lu next: %p }\n", &nodes[i], nodes[i].id, nodes[i].next);
  }
  printf("]\n");
}

//void initshuffle(node nodes[] ) {//, node* locs[]) {
// initialize linked lists.
node** initshuffle(uint64_t size, int num_threads) { //node nodes[] ) {//, node* locs[]) {
  int64_t i = 0;

  //size = size / num_threads;

  node** locs = malloc(sizeof(node*) * size); // node array
  node* nodes = malloc(sizeof(node) * size);  // temporary node pointers
  node** bases = malloc(sizeof(node*) * num_threads); // initial node array

  // initialize ids
  #pragma omp parallel for num_threads(num_threads)
  for(i = 0; i < size; ++i) {
    nodes[i].id = i;
  }

  // randomly permute nodes (fisher-yates (or knuth) shuffle)
  for(i = size-1; i > 0; --i) {
    uint64_t j = random() % i;
    node temp = nodes[i];
    nodes[i] = nodes[j];
    nodes[j] = temp;
  }

  // extract locs
  #pragma omp parallel for num_threads(num_threads)
  for(i = 0; i < size; ++i) {
    locs[ nodes[i].id ] = &nodes[i];
  }

  // initialize pointers
  // chop id-space into num_threads sections and build a circular list for each
  int64_t base = 0;
  #pragma omp parallel for num_threads(num_threads)
  for(i = 0; i < size; ++i) {
    uint64_t id = nodes[i].id;
    int64_t thread_size = size / num_threads;
    int64_t base = id / thread_size;
    uint64_t nextid = nodes[i].id + 1;
    uint64_t wrapped_nextid = (nextid % thread_size) + (base * thread_size);
    //printf("%d: %d %d %d\n", id, base, nextid, wrapped_nextid);
    nodes[i].next = locs[ wrapped_nextid ];
  }

  // grab initial pointers for each list
  for(i = 0; i < num_threads; ++i) {
    bases[i] = locs[i * size / num_threads];
  }

  //print(nodes, size);
  
  free(locs);
  return bases;
}


// walk the list
uint64_t walk( node nodes[], uint64_t count ) {
  uint64_t sum = 0;
  node* i = &nodes[0];

  while (count > 0) {
    sum += (uint64_t)i->next; // do some work to make the optimizer happy
    i = i->next;
    count--;
  }

  return sum;
}


int main(int argc, char* argv[]) {

  assert(argc == 3);
  uint64_t bits = atoi(argv[1]);
  uint64_t num_threads = atoi(argv[2]);
  uint64_t size = (1 << bits);


  printf("Initializing footprint of size %lu * %lu = %lu bytes....\n", size, sizeof(node), size * sizeof(node));
  // initialize random number generator
  srandom(time(NULL));

  //print( data );


  int n;

  node** data = initshuffle( size, num_threads );

  const uint64_t multiplier = 10;
  for(n = 0; n < multiplier; n++) {


    //print( data );
    struct timespec start;
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    //printf("Start time: %d seconds, %d nanoseconds\n", (int) start.tv_sec, (int) start.tv_nsec);

    int m;
    uint64_t base = 0; 
    const uint64_t procsize = size / num_threads;
    uint64_t* results = malloc( sizeof(uint64_t) * num_threads); 
#pragma omp parallel for num_threads(num_threads)
    for(m = 0; m < num_threads; m++) {
      printf("num_threads %d\n", omp_get_num_threads());
      node* d = data[m];
      results[m] = walk( d, procsize );
      //results[m] = walk( d + (m*procsize), procsize*multiplier );
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    //printf("End time: %d seconds, %d nanoseconds\n", (int) end.tv_sec, (int) end.tv_nsec);

    uint64_t result = 0;
    for(m = 0; m < num_threads; m++) {
      result += results[m];
    }

    const uint64_t totalsize = procsize * num_threads;
    if (num_threads == 1) assert(totalsize == size);
    uint64_t runtime_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec) - ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);
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

    printf("{ bits:%lu num_threads:%lu total_bytes:%lu proc_bytes:%lu total_requests:%lu proc_requests:%lu request_rate:%f proc_request_rate:%f data_rate:%f proc_data_rate:%f proc_request_time:%f proc_request_cpu_cycles:%f }\n",
	   bits, num_threads, total_bytes, proc_bytes, total_requests, proc_requests, 
	   request_rate, proc_request_rate, data_rate, proc_data_rate, proc_request_time, proc_request_cpu_cycles);

    printf("(%lu/%lu): %lu nanoseconds, %lu requests, %f req/s, %f req/s/proc, %f ns/req, %f B/s, %f clocks each\n", 
	   bits, num_threads,
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
