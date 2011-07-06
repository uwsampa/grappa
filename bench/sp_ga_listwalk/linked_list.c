
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#include <omp.h>
#include "mpi.h"

#include <sched.h>

#include "linked_list-node.h"
#include "linked_list-alloc.h"
#include "linked_list-walk.h"

#include "thread.h"

#include "GmTypes.hpp"
#include "CoreQueue.hpp"
#include "Delegate.hpp"
#include "SplitPhase.hpp"

#include "numautil.h"

/* 
 * Multiple threads walking linked lists concurrently.
 * Nodes are 64bytes, one cacheline. List nodes are also shuffled to make locality unlikely.
 * Run with option --help for usage.
 */

//#include "ExperimentRunner.h"

void __sched__noop__(pid_t, unsigned int, cpu_set_t*) {}
#define PIN_THREADS 1
#if PIN_THREADS
    #define SCHED_SET sched_setaffinity
#else
    #define SCHED_SET __sched__noop__
#endif

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
	uint64_t listsize;
	SplitPhase* sp;
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
//      info->results[thread_num] = walk_prefetch_switch( me, info->bases + thread_num * num_lists_per_thread, procsize, num_lists_per_thread, 0);
      info->results[thread_num] = walk_split_phase( me,
    		  	  	  	  	  	  	  	  	  	  	info->sp,
    		  	  	  	  	  	  	  	  	  	  	info->bases + thread_num * num_lists_per_thread,
    		  	  	  	  	  	  	  	  	  	  	procsize,
    		  	  	  	  	  	  	  	  	  	  	num_lists_per_thread);
      uint64_t end_tsc;
      rdtscll(end_tsc);
      info->times[thread_num] = end_tsc - start_tsc;

//      printf("core%u-thread%u: FINISHED\n", omp_get_thread_num(), me->id);

     thread_exit(me, NULL);
}


int main(int argc, char* argv[]) {

  //assert(argc >= 5);
  uint64_t bits = 24; //atoi(argv[1]);            // 2^bits=number of nodes
  uint64_t num_threads_per_core = 1; //atoi(argv[2]);     // sw threads/ hw thread
  uint64_t num_cores_per_node = 1; //atoi(argv[3]);// hw threads
  uint64_t size = (1 << bits);              

  uint64_t num_lists_per_thread = 1; //atoi(argv[4]); // number of concurrent accesses per thread	

  int numa_node0 = 0;
  int numa_node1 = 1;

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
      num_threads_per_core = atoi(optarg);
      break;
    case 'c':
      num_cores_per_node = atoi(optarg);
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
  /* if (!use_green_threads && (num_threads_per_core>1)) { */
  /* 	printf("Must use option -g to turn on green threads to specify # threads with -t\n"); */
  /* 	exit(1); */
  /* } */
	
  const uint64_t number_of_repetitions = 1;
  if (number_of_repetitions > 1) assert(false); // XXX not reinitializing right to do more than one experiment


  int max_cpu0;
  unsigned int* cpus0 = numautil_cpusForNode(numa_node0, &max_cpu0);


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

  

  assert(max_cpu0 > num_cores_per_node);


  // GlobalArray initialization
  GA::Initialize(argc, argv);
  
  int rank = GA::nodeid();
  int num_nodes = GA::nodes();  


  uint64_t total_num_threads = num_nodes * num_cores_per_node * num_threads_per_core;


  int64_t vertices_size_in_words = size * sizeof(node) / sizeof(int64_t);
  int64_t bases_size_in_words = num_nodes * num_threads_p

  // initialize random number generator
  srandom(time(NULL));

  if (0 == rank) std::cout << "Start." << std::endl;
    
  GA::GlobalArray vertices = GA::GlobalArray( MT_C_LONGLONG, // type
						1,             // one-dimensional
						&vertices_size_in_words,   // number of elements along each dimension
						"vertices",  // name
						NULL );        // distribute evenly
  
  GA::GlobalArray bases = GA::GlobalArray( MT_C_LONGLONG, // type
					     1,             // one-dimensional
					     &bases_size_in_words,  // number of elements along each dimension
					     "bases",  // name
					     NULL );        // distribute evenly
  
  GA::GlobalArray times = GA::GlobalArray( MT_C_DBL, // type
					     1,             // one-dimensional
					     &num_nodes,  // number of elements along each dimension
					     "times",  // name
					     NULL );        // distribute evenly
 
    int64_t local_begin = 0;
    int64_t local_end = 0;
    int64_t * local_array;
    
    allocate_ga( rank, num_nodes,
		 size, 
         num_cores_per_node*num_threads_per_core,  //num threads per node
         num_lists_per_thread,
		 &vertices, &bases,
		 &local_begin, &local_end, &local_array );


    vertices.printDistribution();


  // split phase and delegate
  CoreQueue<uint64_t>* sock0_qs_fromDel[num_threads_per_node];
  CoreQueue<uint64_t>* sock0_qs_toDel[num_threads_per_node];

  SplitPhase* sp[num_threads_per_node];
  for (int th=0; th<num_threads_per_node; th++) {
      sock0_qs_fromDel[th] = CoreQueue<uint64_t>::createQueue();
      sock0_qs_toDel[th] =  CoreQueue<uint64_t>::createQueue();
      sp[th] = new SplitPhase(sock0_qs_toDel[th], sock0_qs_fromDel[th], num_threads_per_core, 
                                local_array, local_begin, local_end);
  }

    // run number_of_repetitions # of experiments
  for(n = 0; n < number_of_repetitions; n++) {
      
      Delegate delegate(sock0_qs_fromDel, sock0_qs_toDel, num_cores_per_node, &vertices);
	            	  	  
    const uint64_t listsize_per_thread = size / (total_num_threads * num_lists_per_thread);

    struct timespec start;
    struct timespec end;
     
    if (use_green_threads) {
    	// arrays for threads and infos
    	thread* threads[num_threads_per_core][num_threads_per_core];
    	struct  walk_info walk_infos[num_threads_per_core][num_threads_per_core];
    
  	  // for each hw thread, create <num_threads_per_core> green threads
    	#pragma omp parallel for num_threads(num_threads) 
    	for(int core_num = 0; core_num < num_threads; core_num++) {
            cpu_set_t set;
            CPU_ZERO(&set);
    		CPU_SET(cpus0[core_num], &set);
	        SCHED_SET(0, sizeof(cpu_set_t), &set);

	      	#pragma omp critical (crit_create)
	     	 {
	       	 for (int gt = 0; gt<num_threads_per_core; gt++) {
	       	   walk_infos[core_num][gt].num = core_num*num_threads_per_core + gt;
	       	   walk_infos[core_num][gt].results = results;
	       	   walk_infos[core_num][gt].times   = times;
	       	   walk_infos[core_num][gt].bases = bases;
	       	   walk_infos[core_num][gt].num_lists_per_thread = num_lists_per_thread;
	       	   walk_infos[core_num][gt].listsize = listsize_per_thread;
	       	   walk_infos[core_num][gt].sp = sp[core_num];
	       	   threads[core_num][gt] = thread_spawn(masters[core_num], schedulers[core_num], thread_runnable, &walk_infos[core_num][gt]);
	         }
	     	}
    	}

    	// start monitoring and timing    
    	//er.startIfEnabled();
    	clock_gettime(CLOCK_MONOTONIC, &start);
    	//printf("Start time: %d seconds, %d nanoseconds\n", (int) start.tv_sec, (int) start.tv_nsec);

    	// do the work
		omp_set_nested(1);

    	#pragma omp parallel for num_threads(num_dels+1)
    	for (int task=0; task<num_dels+1; task++) {
    		if (task==0) {
				#pragma omp parallel for num_threads(num_threads)
				for (core_num=0; core_num < num_threads; core_num++) {
					cpu_set_t set;
					CPU_ZERO(&set);
					CPU_SET(cpus0[core_num], &set);
	                SCHED_SET(0, sizeof(cpu_set_t), &set);

					run_all(schedulers[core_num]);
				} //barrier

                printf("barrier hit, now will send QUIT sigs\n");

				//QUIT signals
				#pragma omp parallel for num_threads(num_threads)
				for (thread_num=0; thread_num < num_threads; thread_num++) {
					cpu_set_t set;
					CPU_ZERO(&set);

					if (thread_num < thr_per_sock) CPU_SET(cpus0[thread_num], &set);//CPU_SET(thread_num*2,&set);
					else CPU_SET(cpus1[thread_num-thr_per_sock], &set);//CPU_SET((thread_num-thr_per_sock)*2+1,&set);

	                SCHED_SET(0, sizeof(cpu_set_t), &set);

					sp[thread_num]->issue(QUIT, 0, 0, masters[thread_num]);
				}

    		} else { //delegate
    			// assuming last core on sock0 and sock1
    			cpu_set_t set;
    			CPU_ZERO(&set);
    			if (task==1) CPU_SET(cpus0[max_cpu0-1],&set);//CPU_SET(10,&set);
    			else CPU_SET(cpus1[max_cpu1-1],&set);//CPU_SET(11,&set);

                SCHED_SET(0, sizeof(cpu_set_t), &set);

    			delegates[task-1]->run();
    		}
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
	  	for (int gt=0; gt<num_threads_per_core; gt++) {
			destroy_thread(threads[thread_num][gt]);
	  	}
    	        }
    	}
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
    for(thread_num = 0; thread_num < total_num_threads; thread_num++) {
      result += results[thread_num];
      latency_ticks += times[thread_num];
      printf("thread %lu: %lu ticks / %lu procsize = %f ticks per request\n", thread_num, times[thread_num], procsize, (double) times[thread_num]/procsize);
    }

    //uint64_t runtime_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec) - ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);


    const double ticks_per_ns = 2.67;
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
    const double   avg_latency_ticks = (double)latency_ticks / (total_requests / num_lists_per_thread);// total_num_threads*num req per thread = total requests
    const double   avg_latency_ns = (double)avg_latency_ticks / ticks_per_ns;
    //what does this mean?-->const double   avg_latency_ns = (double)runtime_ns / (proc_requests / num_lists_per_thread);

    printf("{'bits':%lu, 'st_per_ht':%lu, 'use_green_threads':%d, 'num_threads':%lu, 'concurrent_reads':%lu, 'total_bytes':%lu, 'proc_bytes':%lu, 'total_requests':%lu, 'proc_requests':%lu, 'request_rate':%f, 'proc_request_rate':%f, 'data_rate':%f, 'proc_data_rate':%f, 'proc_request_time':%f, 'proc_request_cpu_cycles':%f, 'avg_latency_ticks':%f }\n",
	   bits, num_threads_per_core, use_green_threads, num_threads, num_lists_per_thread, total_bytes, proc_bytes, total_requests, proc_requests, 
	   request_rate, proc_request_rate, data_rate, proc_data_rate, proc_request_time, proc_request_cpu_cycles, avg_latency_ticks);

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

    free(results);
    free(times);
  }
  //print( data );

  return 0;
}
