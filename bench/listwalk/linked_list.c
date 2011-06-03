
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
#include "linked_list-delegate.h"

#include "thread.h"

#include "MCRingBuffer.h"

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

  // local data for each software thread
  struct walk_info {
	uint64_t num;
	uint64_t* results;
	uint64_t* times; 
	node** bases;
	uint64_t num_lists_per_thread;
	uint64_t procsize;
  };

void thread_runnable(thread* me, void* arg) {
      // no synchronization on shared data because interleaving is controlled by yield
      // and threads do not run parallel
      struct walk_info* info = (struct walk_info*) arg;
      uint64_t thread_num = info->num;
      uint64_t num_lists_per_thread = info->num_lists_per_thread;
      uint64_t procsize = info->procsize;

      uint64_t start_tsc;
      rdtscll(start_tsc);
      //results[thread_num] = walk( data, thread_num, num_threads, num_lists, procsize );
      //results[thread_num] = multiwalk( bases + thread_num * num_lists_per_thread,
      //                                num_lists_per_thread, procsize );
      info->results[thread_num] = walk_prefetch_switch( me, info->bases + thread_num * num_lists_per_thread, procsize, num_lists_per_thread, 0);
      uint64_t end_tsc;
      rdtscll(end_tsc);
      info->times[thread_num] = end_tsc - start_tsc;

     thread_exit(me, NULL);
}


int main(int argc, char* argv[]) {

  //assert(argc >= 5);
  uint64_t bits = 24; //atoi(argv[1]);            // 2^bits=number of nodes
  uint64_t green_per_ht = 1; //atoi(argv[2]);     // sw threads/ hw thread
  uint64_t num_threads = 1; //atoi(argv[3]);// hw threads
  uint64_t size = (1 << bits);              

  uint64_t num_lists_per_thread = 1; //atoi(argv[4]); // number of concurrent accesses per thread	

  // optional 4th arg "-i" indicates to use ExperimentRunner
  int do_monitor = 0;
/*  if (argc==6) {
	if (strncmp("-i", argv[5], 2)==0) {
		do_monitor = 1;
        }
   }*/
  
  int use_green_threads = 0;
  int use_jump_threads = 0;

  static struct option long_options[] = {
    {"bits",             required_argument, NULL, 'b'},
    {"threads_per_core", required_argument, NULL, 't'},
    {"cores",            required_argument, NULL, 'c'},
    {"lists_per_thread", required_argument, NULL, 'l'},
    {"monitor",          no_argument,       NULL, 'm'},
    {"green_threads",    no_argument,       NULL, 'g'},
    {"jump_threads",     no_argument,       NULL, 'j'},
    {"help",             no_argument,       NULL, 'h'},
    {NULL,               no_argument,       NULL, 0}
  };
  int c, option_index = 1;
  while ((c = getopt_long(argc, argv, "b:t:c:s:l:m:gjh?",
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
    case 'j':
      use_jump_threads = 1;
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

  /* // must turn on green threads to allow more than one thread per core */
  /* if (!use_green_threads && (green_per_ht>1)) { */
  /* 	printf("Must use option -g to turn on green threads to specify # threads with -t\n"); */
  /* 	exit(1); */
  /* } */
	
  const uint64_t number_of_repetitions = 10;


  // num threads is total software threads over all hts
  uint64_t total_num_threads = green_per_ht * num_threads;

  // initialize random number generator
  srandom(time(NULL));

  //print( data );


  int n;

  // initialize data structure
  node** bases = allocate_page( size, total_num_threads, num_lists_per_thread );


  // experiment runner init and enable
  //ExperimentRunner er;
  //if (do_monitor) er.enable();

  // create masters and schedulers for green threads
  thread* masters[num_threads];
  scheduler* schedulers[num_threads];

  if (use_green_threads) {
      #pragma omp parallel for num_threads(num_threads)
      for (int i = 0; i<num_threads; i++) {
        #pragma omp critical (crit_init)
        {
  	  // create master thread and scheduler 
  	  thread* master = thread_init();
  	  scheduler* sched = create_scheduler(master);
          masters[i] = master;
	  schedulers[i] = sched;
	}
      }
  }

  // run number_of_repetitions # of experiments
  for(n = 0; n < number_of_repetitions; n++) {
    //print( data );

    uint64_t thread_num;
    const uint64_t procsize = size / (total_num_threads * num_lists_per_thread);
    uint64_t* results = (uint64_t*) malloc( sizeof(uint64_t) * total_num_threads * num_lists_per_thread); 

    uint64_t* times = (uint64_t*) malloc( sizeof(uint64_t) * total_num_threads );

    struct timespec start;
    struct timespec end;
     
    if (use_green_threads) {
    	// arrays for threads and infos
    	thread* threads[num_threads][green_per_ht];
    	struct  walk_info walk_infos[num_threads][green_per_ht];
    
  	  // for each hw thread, create <green_per_ht> green threads
    	#pragma omp parallel for num_threads(num_threads) 
    	for(thread_num = 0; thread_num < num_threads; thread_num++) {
      	#pragma omp critical (crit_create)
     	 {
       	 for (int gt = 0; gt<green_per_ht; gt++) {
       	   walk_infos[thread_num][gt].num = thread_num*green_per_ht + gt;
       	   walk_infos[thread_num][gt].results = results;
       	   walk_infos[thread_num][gt].times   = times;
       	   walk_infos[thread_num][gt].bases = bases;
       	   walk_infos[thread_num][gt].num_lists_per_thread = num_lists_per_thread;
       	   walk_infos[thread_num][gt].procsize = procsize;
       	   threads[thread_num][gt] = thread_spawn(masters[thread_num], schedulers[thread_num], thread_runnable, &walk_infos[thread_num][gt]);
         }
      	}
    	}

    	// start monitoring and timing    
    	//er.startIfEnabled();
    	clock_gettime(CLOCK_MONOTONIC, &start);
    	//printf("Start time: %d seconds, %d nanoseconds\n", (int) start.tv_sec, (int) start.tv_nsec);

    	// do the work
    	#pragma omp parallel for num_threads(num_threads)
    	for (thread_num=0; thread_num < num_threads; thread_num++) {
    		run_all(schedulers[thread_num]);
    	}

    	// stop timing
    	clock_gettime(CLOCK_MONOTONIC, &end);
    	//er.stopIfEnabled();
    	//printf("End time: %d seconds, %d nanoseconds\n", (int) end.tv_sec, (int) end.tv_nsec);
    
   	 // cleanup threads (not sure if each respective master has to do it, so being safe by letting each master call destroy on its green threads)
    	#pragma omp parallel for num_threads(num_threads)
    	for (thread_num = 0; thread_num < num_threads; thread_num++) {
		#pragma omp critical (crit_destroy)
        	{
	  	for (int gt=0; gt<green_per_ht; gt++) {
			destroy_thread(threads[thread_num][gt]);
	  	}
    	        }
    	}
    } else if (use_jump_threads) { // NOT use_green_threads
      // start monitoring and timing    
      //er.startIfEnabled();
      clock_gettime(CLOCK_MONOTONIC, &start);
      //printf("Start time: %d seconds, %d nanoseconds\n", (int) start.tv_sec, (int) start.tv_nsec);
      
      // do work
      jthr_memdesc* memdescs = malloc( sizeof(jthr_memdesc) * num_threads * num_lists_per_thread );
      MCRingBuffer* send_queues = malloc( sizeof(MCRingBuffer) * num_threads );
      MCRingBuffer* receive_queues = malloc( sizeof(MCRingBuffer) * num_threads );
#pragma omp parallel for 
      for(int i = 0; i < num_threads; i++) {
	MCRingBuffer_init( &send_queues[i] );
	MCRingBuffer_init( &receive_queues[i] );
      }


#pragma omp parallel for num_threads(num_threads+1)
      for(thread_num = 0; thread_num <= num_threads; thread_num++) {
	if (thread_num < num_threads) {
	  uint64_t start_tsc;
	  rdtscll(start_tsc);
	  
	  results[thread_num] = walk_jump( bases + thread_num * num_lists_per_thread, 
					   procsize, num_lists_per_thread, 0, 
					   memdescs + thread_num * num_lists_per_thread,
					   send_queues + thread_num,
					   receive_queues + thread_num );
      	
	  uint64_t end_tsc;
	  rdtscll(end_tsc);
	  times[thread_num] = end_tsc - start_tsc;
	} else {

	  delegate_jump( memdescs, send_queues, receive_queues, 
			 size, procsize, num_lists_per_thread, num_threads );

    	}
      }
      // stop timing
      clock_gettime(CLOCK_MONOTONIC, &end);
      //er.stopIfEnabled();
      //printf("End time: %d seconds, %d nanoseconds\n", (int) end.tv_sec, (int) end.tv_nsec);


    } else { // NOT use_green_threads and not use_jump_threads
      // start monitoring and timing    
	//er.startIfEnabled();
    	clock_gettime(CLOCK_MONOTONIC, &start);
    	//printf("Start time: %d seconds, %d nanoseconds\n", (int) start.tv_sec, (int) start.tv_nsec);

    	// do work
    	#pragma omp parallel for num_threads(num_threads)
    	for(thread_num = 0; thread_num < num_threads; thread_num++) {

      	    uint64_t start_tsc;
      	    rdtscll(start_tsc);
      	
	    results[thread_num] = walk( bases + thread_num * num_lists_per_thread, procsize, num_lists_per_thread, 0);
      	
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

    printf("(%lu/%lu): %f MB/s, %g ticks avg, %g ns avg, %lu nanoseconds, %lu requests, %f Mreq/s, %f Mreq/s/proc, %f ns/req, %f B/s, %f clocks each\n", 
	   bits, num_threads,
	   (double)total_bytes/runtime_s/1000/1000, 
           avg_latency_ticks, avg_latency_ns,
	   runtime_ns, totalsize, (double)totalsize/((double)runtime_ns/1000000000)/1000000, 
	   (double)totalsize/((double)runtime_ns/1000000000)/num_threads/1000000, 
	   (double)runtime_ns/totalsize, 
	   (double)totalsize*sizeof(node)/((double)runtime_ns/1000000000),
	   ((double)runtime_ns/(totalsize/num_threads)) / (.00000000044052863436 * 1000000000) 
	   );
  }
  //print( data );

  return 0;
}
