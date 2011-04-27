
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#include <omp.h>

#include "linked_list-node.h"
#include "linked_list-alloc.h"
#include "linked_list-walk.h"

#include "thread.h"

#include <convey/usr/cny_comp.h>

/* 
 * Multiple threads walking linked lists concurrently.
 * Nodes are 64bytes, one cacheline. List nodes are also shuffled to make locality unlikely.
 * Run with option --help for usage.
 */

//#include "ExperimentRunner.h"

#define rdtscll(val) do { \
  unsigned int __a,__d; \
  asm volatile("rdtsc" : "=a" (__a), "=d" (__d)); \
  (val) = ((unsigned long)__a) | (((unsigned long)__d)<<32); \
  } while(0)


extern long walk();

int main(int argc, char* argv[]) {

//// ONE THREAD allowed only

  //assert(argc >= 5);
  uint64_t bits = 24; //atoi(argv[1]);            // 2^bits=number of nodes
  uint64_t green_per_ht = 1; //atoi(argv[2]);     // sw threads/ hw thread
  uint64_t num_threads = 1; //atoi(argv[3]);// hw threads
  uint64_t size = (1 << bits);              

  uint64_t num_lists_per_thread = 16; //atoi(argv[4]); // number of concurrent accesses per thread	

  // optional 4th arg "-i" indicates to use ExperimentRunner
  int do_monitor = 0;
/*  if (argc==6) {
	if (strncmp("-i", argv[5], 2)==0) {
		do_monitor = 1;
        }
   }*/
  
  int use_green_threads = 0;

  static struct option long_options[] = {
    {"bits",             required_argument, NULL, 'b'},
    {"threads_per_core", required_argument, NULL, 't'},
    {"cores",            required_argument, NULL, 'c'},
    {"lists_per_thread", required_argument, NULL, 'l'},
    {"monitor",          no_argument,       NULL, 'm'},
    {"green_threads",    no_argument,       NULL, 'g'},
    {"help",             no_argument,       NULL, 'h'},
    {NULL,               no_argument,       NULL, 0}
  };
  int c, option_index = 1;
  while ((c = getopt_long(argc, argv, "b:t:c:s:l:m:gh?",
                          long_options, &option_index)) >= 0) {
    switch (c) {
    case 0:   // flag set
      break;
    case 'b':
      bits = atoi(optarg);
      size = (1<<bits);
      break;
    case 't':
      green_per_ht = atoi(optarg);
      break;
    case 'c':
      num_threads = atoi(optarg);
      break;
    case 'l':
      num_lists_per_thread = atoi(optarg);
      break;
    case 'm':
      do_monitor = 1;
      break;
    case 'g':
      use_green_threads = 1;
      break;
    case 'h':
    case '?':
    default:
      printf("Available options:\n");
    for (struct option* optp = &long_options[0]; optp->name != NULL; ++optp) {
      if (optp->has_arg == no_argument) {
        printf("  -%c, --%s\n", optp->val, optp->name);
      } else {
        printf("  -%c, --%s <ARG>\n", optp->val, optp->name);
      }
    }
    exit(1);
    }
  }

  // must turn on green threads to allow more than one thread per core
  if (!use_green_threads && (green_per_ht>1)) {
	printf("Must use option -g to turn on green threads to specify # threads with -t\n");
	exit(1);
  }

// convey
 if (argc == 1)
    size = 100;		// default size
  else if (argc == 2) {
    size = atoi(argv[1]);
    if (size > 0) {
      printf("Running UserApp.exe with size = %lld\n", size);
    } else {
      usage (argv[0]);
      return 0;
    }
  }
  else {
    usage (argv[0]);
    return 0;
  }

  printf("Getting signature\n");

  // Get PDK signature
  cny_image_t        sig2;
  cny_image_t        sig;
  if (cny$get_signature_fptr)
    (*cny$get_signature_fptr) ("listwalk", &sig, &sig2);
  else 
    fprintf(stderr,"where is my get_sig ptr\n");
 
  // Sample program 1
  // 1.  Allocate coproc memory for 2 arrays of numbers 
  // 2.  Populate arrays with 64-bit integers
  // 3.  Call coproc routine to sum the numbers
  printf("Running Sample Program 1\n");

  // check interleave is binary
  if (cny_cp_interleave() == CNY_MI_3131) {
    printf("ERROR - interleave set to 3131, this personality requires binary interleave\n");
    exit (1);
  }


  const uint64_t number_of_repetitions = 1;


  // num threads is total software threads over all hts
  uint64_t total_num_threads = green_per_ht * num_threads;

  // initialize random number generator
  srandom(time(NULL));

  //print( data );


  int n;

  // initialize data structure
  node** bases = allocate_heap( size, total_num_threads, num_lists_per_thread );



  // run number_of_repetitions # of experiments
  for(n = 0; n < number_of_repetitions; n++) {
    //print( data );

    uint64_t thread_num;
    const uint64_t procsize = size / (total_num_threads * num_lists_per_thread);
    uint64_t* results = (uint64_t*) malloc( sizeof(uint64_t) * total_num_threads * num_lists_per_thread); 

    uint64_t* times = (uint64_t*) malloc( sizeof(uint64_t) * total_num_threads );

    struct timespec start;
    struct timespec end;
     
	// start monitoring and timing    
    	clock_gettime(CLOCK_MONOTONIC, &start);
    	//printf("Start time: %d seconds, %d nanoseconds\n", (int) start.tv_sec, (int) start.tv_nsec);

    	// do work
    	for(thread_num = 0; thread_num < num_threads; thread_num++) {

      	    uint64_t start_tsc;
      	    rdtscll(start_tsc);
      
      // blocking call to do the walk
      uint64_t* result;
      results[thread_num] = l_copcall_fmt(sig, walk, "AAAA", bases+thread_num*num_lists_per_thread, num_lists_per_thread, procsize, result);	
	    //results[thread_num] = walk( bases + thread_num * num_lists_per_thread, procsize, num_lists_per_thread, 0);
      	
	    uint64_t end_tsc;
      	    rdtscll(end_tsc);
      	    times[thread_num] = end_tsc - start_tsc;
    	}

    	// stop timing
    	clock_gettime(CLOCK_MONOTONIC, &end);
    	//er.stopIfEnabled();
    	//printf("End time: %d seconds, %d nanoseconds\n", (int) end.tv_sec, (int) end.tv_nsec);
    }


    uint64_t runtime_ns = 0;
    uint64_t latency_ticks = 0;
    //times[thread_num] = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec) - ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);
    runtime_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec) - ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);

    // unimportant calculation to ensure calls don't get optimized away
    uint64_t result = 0;
    for(thread_num = 0; thread_num < num_threads; thread_num++) {
      result += results[thread_num];
      latency_ticks += times[thread_num];
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
    const double   avg_latency_ticks = (double)latency_ticks / (total_requests / num_lists_per_thread);
    const double   avg_latency_ns = (double)runtime_ns / (proc_requests / num_lists_per_thread);

    printf("{'bits':%lu, 'st_per_ht':%lu, 'use_green_threads':%d, 'num_threads':%lu, 'concurrent_reads':%lu, 'total_bytes':%lu, 'proc_bytes':%lu, 'total_requests':%lu, 'proc_requests':%lu, 'request_rate':%f, 'proc_request_rate':%f, 'data_rate':%f, 'proc_data_rate':%f, 'proc_request_time':%f, 'proc_request_cpu_cycles':%f}\n",
	   bits, green_per_ht, use_green_threads, num_threads, num_lists_per_thread, total_bytes, proc_bytes, total_requests, proc_requests, 
	   request_rate, proc_request_rate, data_rate, proc_data_rate, proc_request_time, proc_request_cpu_cycles);

    printf("(%lu/%lu): %f MB/s, %g ticks avg, %g ns avg, %lu nanoseconds, %lu requests, %f req/s, %f req/s/proc, %f ns/req, %f B/s, %f clocks each\n", 
	   bits, num_threads,
	   (double)total_bytes/runtime_s/1000/1000, 
           avg_latency_ticks, avg_latency_ns,
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
