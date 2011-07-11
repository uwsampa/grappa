
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

#include <sched.h>

#include "node.hpp"
#include "linked_list-walk.h"

#include "thread.h"

#include "GmTypes.hpp"
#include "CoreQueue.hpp"
#include "Delegate.hpp"
#include "SplitPhase.hpp"

#include "numautil.h"

#include "mpi.h"
#include "ga++.h"
#include "ga_alloc.hpp"

/* 
 * Multiple threads walking linked lists concurrently.
 * Nodes are 64bytes, one cacheline. List nodes are also shuffled to make locality unlikely.
 * Run with option --help for usage.
 */

//#include "ExperimentRunner.h"

void __sched__noop__(pid_t, unsigned int, cpu_set_t*) {}
#define PIN_THREADS 0
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
	int64_t list_id;
	int64_t result;
	uint64_t time; 
	int64_t  base;
	uint64_t num_lists_per_thread;
	uint64_t count;
	SplitPhase* sp;
  };

void thread_runnable(thread* me, void* arg) {
      struct walk_info* info = (struct walk_info*) arg;
      uint64_t count = info->count;
      uint64_t num_lists_per_thread = 1;// XXX info->num_lists_per_thread;
      
      uint64_t start_tsc;
      rdtscll(start_tsc);
      
//printf("nlpt = %lu, listsize=%lu\n", num_lists_per_thread, listsize);
      info->result = walk_split_phase( me,
                                    info->sp,
                                    info->base,
                                    count,
                                    num_lists_per_thread);
      uint64_t end_tsc;
      rdtscll(end_tsc);
      info->time = end_tsc - start_tsc;

//      printf("core%u-thread%u: FINISHED\n", omp_get_thread_num(), me->id);

     thread_exit(me, NULL);
}


int main(int argc, char* argv[]) {

  //assert(argc >= 5);
  uint64_t bits = 24; //atoi(argv[1]);            // 2^bits=number of nodes
  uint64_t num_threads_per_core = 1; //atoi(argv[2]);     // sw threads/ hw thread
  uint64_t num_cores_per_node = 1; //atoi(argv[3]);// hw threads
  uint64_t size = (1 << bits);
  uint64_t num_traversals = 1; // number of traversals thru the circular lists         

  uint64_t num_lists_per_thread = 1; //atoi(argv[4]); // number of concurrent accesses per thread	

  int numa_node0 = 0;

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
    {"cores_per_node",            required_argument, NULL, 'c'},
    {"lists_per_thread", required_argument, NULL, 'l'},
    {"num_traversals_log",  required_argument, NULL, 'a'},
    {"monitor",          no_argument,       NULL, 'm'},
    {"green_threads",    no_argument,       NULL, 'g'},
    {"jump_threads",     no_argument,       NULL, 'j'},
    {"help",             no_argument,       NULL, 'h'},
    {NULL,               no_argument,       NULL, 0}
  };
  int c, option_index = 1;
  while ((c = getopt_long(argc, argv, "b:t:c:s:l:a:m:gjh?",
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
    case 'a':
      num_traversals = (1<<atoi(optarg));
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
  thread* masters[num_cores_per_node];
  scheduler* schedulers[num_cores_per_node];

  if (use_green_threads) {
      #pragma omp parallel for num_threads(num_cores_per_node)
      for (uint64_t i = 0; i<num_cores_per_node; i++) {
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

  
#if PIN_THREADS
  assert((unsigned int)max_cpu0 > num_cores_per_node); 
#endif

  // GlobalArray initialization
  GA::Initialize(argc, argv);
  
  int rank = GA::nodeid();
  int num_nodes = GA::nodes();  


  uint64_t total_num_threads = num_nodes * num_cores_per_node * num_threads_per_core;


  int64_t vertices_size_in_words = size * sizeof(node) / sizeof(int64_t);
  int64_t bases_size_in_words = num_nodes * num_cores_per_node * num_threads_per_core; // TODO num list per thread too

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
  CoreQueue<uint64_t>* sock0_qs_fromDel[num_cores_per_node];
  CoreQueue<uint64_t>* sock0_qs_toDel[num_cores_per_node];

  SplitPhase* sp[num_cores_per_node];
  for (uint64_t th=0; th<num_cores_per_node; th++) {
      sock0_qs_fromDel[th] = CoreQueue<uint64_t>::createQueue();
      sock0_qs_toDel[th] =  CoreQueue<uint64_t>::createQueue();
      sp[th] = new SplitPhase(sock0_qs_toDel[th], sock0_qs_fromDel[th], num_threads_per_core, 
                                local_array, local_begin<<3, local_end<<3);
  }

  MPI_Barrier( MPI_COMM_WORLD );

    // run number_of_repetitions # of experiments
  for(uint64_t n = 0; n < number_of_repetitions; n++) {
      
      Delegate delegate(sock0_qs_fromDel, sock0_qs_toDel, num_cores_per_node, &vertices);
	            	  	  
    const uint64_t listsize_per_thread = size / (total_num_threads * num_lists_per_thread);
    const uint64_t count = listsize_per_thread * num_traversals;

    struct timespec start_time;
    struct timespec end_time;
     
    if (use_green_threads) {
    	// arrays for threads and infos
    	thread* threads[num_threads_per_core][num_threads_per_core];
    	struct  walk_info walk_infos[num_threads_per_core][num_threads_per_core];
    
  	  // for each hw thread, create <num_threads_per_core> green threads
    	#pragma omp parallel for num_threads(num_cores_per_node) 
    	for(uint64_t core_num = 0; core_num < num_cores_per_node; core_num++) {
            cpu_set_t set;
            CPU_ZERO(&set);
    		CPU_SET(cpus0[core_num], &set);
	        SCHED_SET(0, sizeof(cpu_set_t), &set);

	      	#pragma omp critical (crit_create)
	     	 {
	       	 for (uint64_t gt = 0; gt<num_threads_per_core; gt++) {
               int64_t list_id = (rank * num_cores_per_node * num_threads_per_core) + (core_num*num_threads_per_core + gt);
	       	   walk_infos[core_num][gt].list_id = list_id;
               int64_t base;
               bases.get(&list_id, &list_id, &base, NULL);
	       	   walk_infos[core_num][gt].base = base;
               walk_infos[core_num][gt].num_lists_per_thread = num_lists_per_thread;
	       	   walk_infos[core_num][gt].count = count;
	       	   walk_infos[core_num][gt].sp = sp[core_num];
	       	   threads[core_num][gt] = thread_spawn(masters[core_num], schedulers[core_num], thread_runnable, &walk_infos[core_num][gt]);
	         }
	     	}
    	}


    	clock_gettime(CLOCK_MONOTONIC, &start_time);
        MPI_Barrier( MPI_COMM_WORLD );

    	// start monitoring and timing    
    	//er.startIfEnabled();
    	//printf("Start time: %d seconds, %d nanoseconds\n", (int) start.tv_sec, (int) start.tv_nsec);

    	// do the work
		omp_set_nested(1);
        int num_tasks = 2;

    	#pragma omp parallel for num_threads(num_tasks)
    	for (int task=0; task<num_tasks; task++) {
    		if (task==0) {
				#pragma omp parallel for num_threads(num_cores_per_node)
				for (uint64_t core_num=0; core_num < num_cores_per_node; core_num++) {
					cpu_set_t set;
					CPU_ZERO(&set);
					CPU_SET(cpus0[core_num], &set);
	                SCHED_SET(0, sizeof(cpu_set_t), &set);

					run_all(schedulers[core_num]);
				} //barrier

                // no MPI barrier needed since delegate is not servicing remote requests and can just be shut down when local stuff done
                printf("proc%d barrier hit, now will send QUIT sigs\n", rank);

				//QUIT signals
				#pragma omp parallel for num_threads(num_cores_per_node)
				for (uint64_t thread_num=0; thread_num < num_cores_per_node; thread_num++) {
					cpu_set_t set;
					CPU_ZERO(&set);
                    CPU_SET(cpus0[thread_num], &set);
	                SCHED_SET(0, sizeof(cpu_set_t), &set);

					sp[thread_num]->issue(QUIT, 0, 0, masters[thread_num]);
				}

    		} else { //delegate
    			// assuming last core on socket
    			cpu_set_t set;
    			CPU_ZERO(&set);
    			CPU_SET(cpus0[max_cpu0-1],&set);//CPU_SET(10,&set);
    			SCHED_SET(0, sizeof(cpu_set_t), &set);

    			delegate.run();
    		}
    	}

    	// stop timing
        MPI_Barrier( MPI_COMM_WORLD );
    	clock_gettime(CLOCK_MONOTONIC, &end_time);
    	//er.stopIfEnabled();
   
        // unimportant calculation to ensure calls don't get optimized away
        uint64_t result = 0;
        for(uint64_t core_num = 0; core_num < num_cores_per_node; core_num++) {
            for (uint64_t gt=0; gt<num_threads_per_core; gt++) {
               result += walk_infos[core_num][gt].result;
            }
        } 
        printf("result: %lu\n", result);
    
   /* 
   	    // cleanup threads (not sure if each respective master has to do it, so being safe by letting each master call destroy on its green threads)
    	#pragma omp parallel for num_threads(num_cores_per_node)
    	for (uint64_t core_num = 0; core_num < num_cores_per_node; core_num++) {
	    	#pragma omp critical (crit_destroy)
        	{
	  	        for (uint64_t gt=0; gt<num_threads_per_core; gt++) {
			        destroy_thread(threads[core_num][gt]);
	  	        }
    	    }
    	}
*/

    } else { // NOT use_green_threads and not use_jump_threads
        assert(false); //unimplemented
    }


    // local and remote request counts
    uint64_t loc_count=0;
    uint64_t rem_count=0;
    for (uint64_t core_num=0; core_num < num_cores_per_node; core_num++) {
        loc_count+=sp[core_num]->local_req_count;
        rem_count+=sp[core_num]->remote_req_count;
    }

    uint64_t glob_loc_count = 0;
    uint64_t glob_rem_count = 0;
    MPI_Reduce( &loc_count, &glob_loc_count, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD );
    MPI_Reduce( &rem_count, &glob_rem_count, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD );
    uint64_t glob_tot_count = glob_loc_count + glob_rem_count;

    if (0 == rank) std::cout << "local reqs:" << glob_loc_count << "/"<< (double)glob_loc_count/glob_tot_count << " remote reqs:" << glob_rem_count << "/" << (double)glob_rem_count/glob_tot_count  << std::endl;

    // runtime
    double runtime = (end_time.tv_sec + 1.0e-9 * end_time.tv_nsec) - (start_time.tv_sec + 1.0e-9 * start_time.tv_nsec);
    double min_runtime = 0.0;
    MPI_Reduce( &runtime, &min_runtime, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD );
    double max_runtime = 0.0;
    MPI_Reduce( &runtime, &max_runtime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD );

    double rate = count * num_lists_per_thread * num_threads_per_core * num_cores_per_node * num_nodes / min_runtime;

    std::cout << "node " << rank << " runtime is " << runtime << std::endl;

    if (0 == rank) std::cout << "rate is " << rate / 1000000.0 << " Mref/s" << std::endl;    

    MPI_Barrier( MPI_COMM_WORLD ); 

    if (0 == rank) printf("{'bits':%lu, 'num_nodes':%d, 'num_threads_per_core':%lu, 'use_green_threads':%d, 'num_cores_per_node':%lu, 'concurrent_reads':%lu, 'request_rate':%f, 'num_traversals':%lu, 'runtime':%f}\n", bits, num_nodes, num_threads_per_core, use_green_threads, num_cores_per_node, num_lists_per_thread, rate, num_traversals, runtime);

  }

  GA::Terminate();
  return 0;
}
