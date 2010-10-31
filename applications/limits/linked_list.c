
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include <omp.h>

#include "node.h"
#include "alloc.h"
#include "walk.h"

#include "ExperimentRunner.h"

#define NPROC 1
#define BITS 23

#define SIZE (1 << BITS)




int main(int argc, char* argv[]) {

  assert(argc >= 4);
  uint64_t bits = atoi(argv[1]);            // 2^bits=number of nodes
  uint64_t num_threads = atoi(argv[2]);     // number of threads
  uint64_t size = (1 << bits);              

  uint64_t num_lists_per_thread = atoi(argv[3]); // number of concurrent accesses per thread	

  // optional 4th arg "-i" indicates to use ExperimentRunner
  bool do_monitor = false;
  if (argc==5) {
	if (strncmp("-i", argv[4], 2)==0) {
		do_monitor = true;
        }
   }

  const uint64_t number_of_repetitions = 10;


  // initialize random number generator
  srandom(time(NULL));

  //print( data );


  int n;

  // initialize data structure
  node** bases = allocate_page( size, num_threads, num_lists_per_thread );


  // experiment runner init and enable
  ExperimentRunner er;
  if (do_monitor) er.enable();

  for(n = 0; n < number_of_repetitions; n++) {
    //print( data );

    int thread_num;
    const uint64_t procsize = size / (num_threads * num_lists_per_thread);
    uint64_t* results = malloc( sizeof(uint64_t) * num_threads * num_lists_per_thread); 

    uint64_t* times = malloc( sizeof(uint64_t) * num_threads );

    struct timespec start;
    struct timespec end;
      
    // start monitoring and timing    
    er.startIfEnabled();
    clock_gettime(CLOCK_MONOTONIC, &start);
    //printf("Start time: %d seconds, %d nanoseconds\n", (int) start.tv_sec, (int) start.tv_nsec);

    // do work
    #pragma omp parallel for num_threads(num_threads)
    for(thread_num = 0; thread_num < num_threads; thread_num++) {

      //results[thread_num] = walk( data, thread_num, num_threads, num_lists, procsize );
      //results[thread_num] = multiwalk( bases + thread_num * num_lists_per_thread,
      //                                num_lists_per_thread, procsize );
      results[thread_num] = walk( bases + thread_num * num_lists_per_thread, procsize, num_lists_per_thread, 0);

    }

    // stop timing
    clock_gettime(CLOCK_MONOTONIC, &end);
    er.stopIfEnabled();
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
