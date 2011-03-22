
//#define _GNU_SOURCE

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <sched.h>
#include <numa.h>
#include <hugetlbfs.h>
#include <getopt.h>



//#include <xmmintrin.h>

#include <sw_queue_greenery.h>
#include <thread.h>

void setaffinity(int i) {
  cpu_set_t set;
  CPU_ZERO(&set);
  if (i < 6) {
    CPU_SET(i*2,&set);
  } else {
    CPU_SET((i-6)*2+1,&set);
  }
  sched_setaffinity(0, sizeof(cpu_set_t), &set);
  CPU_ZERO(&set);
  sched_getaffinity(0, sizeof(cpu_set_t), &set);
}

typedef struct consumer_args {
  SW_Queue q;
  uint64_t max;
  uint64_t counter;
} consumer_args;

void consumer(thread* me, void * void_args) {
  consumer_args* args = (consumer_args*) void_args;
  uint64_t max = args->max;
  SW_Queue q = args->q;

  while( max-- ) {
    uint64_t val = sq_consume(q, me);
    (*((uint64_t*) val))++;
  }
}

void producer(thread* me, void * void_args) {
  consumer_args* args = (consumer_args*) void_args;
  uint64_t max = args->max;
  uint64_t counter = args->counter;
  SW_Queue q = args->q;

  while (max--) {
    sq_produce(q, counter, me);
  }
  sq_flushQueue(q);
}

void result(int human_readable, uint64_t num_cores, uint64_t iters, 
	    char* model,
	    uint64_t runtime_ns, 
	    uint64_t result, uint64_t sum) {
  double runtime_per_iter = (double)runtime_ns/(iters);
  double runtime_per_inc = (double)runtime_ns/(num_cores * iters);
  if (human_readable)
    printf("num_cores, iters, "
	   "model, "
	   "runtime_ns, runtime_per_iter, runtime_per_inc, throughput, result, sum\n");
  printf("%g, %g, "
	 "%s, "
	 "%g, %g, %g, %g, %g, %g\n",
	 (double)num_cores, (double)iters, 
	 model,
	 (double)runtime_ns, runtime_per_iter, runtime_per_inc, 1000000000.0/runtime_per_inc, 
	 (double)result, (double)sum);
}

int main(int argc, char* argv[]) {
  int human_readable = 0;
  uint64_t num_cores = 6;
  uint64_t iters = 1<<27;

  int dflt = 0;

  int racy_single = dflt,
    racy_double = dflt,
    racy_store_fence = dflt,
    unrelated_full_fence = dflt,
    racy_full_fence = dflt,
    racy_atomic = dflt,
    unrelated_atomic = dflt,
    atomic = dflt,
    atomic_unrelated = dflt,
    atomic_unrelated_load = dflt,
    unrelated = dflt,
    cas = dflt,
    delegate = dflt,
    racy_with_delegate = dflt;

  /* uint64_t racy_runtime_ns, atomic_runtime_ns, cas_runtime_ns, racy_result, atomic_result, cas_result, racy_sum, atomic_sum, cas_sum; */
  /* double racy_runtime_per_iter, racy_runtime_per_inc, atomic_runtime_per_iter, atomic_runtime_per_inc, cas_runtime_per_iter, cas_runtime_per_inc; */


  static struct option long_options[] = {
    {"num_cores",              required_argument, NULL, 'c'},
    {"iters_log",      required_argument, NULL, 'i'},

    {"racy_single",             no_argument,       NULL, 'a'},
    {"racy_double",             no_argument,       NULL, 'b'},
    {"racy_store_fence",             no_argument,       NULL, 'x'},
    {"unrelated_store_fence",             no_argument,       NULL, 'y'},
    {"racy_full_fence",             no_argument,       NULL, 'e'},
    {"racy_atomic",             no_argument,       NULL, 'f'},
    {"unrelated_atomic",             no_argument,       NULL, 'g'},
    {"atomic",             no_argument,       NULL, 'j'},
    {"atomic_unrelated",             no_argument,       NULL, 'n'},
    {"atomic_unrelated_load",             no_argument,       NULL, 'p'},
    {"unrelated",             no_argument,       NULL, 'o'},
    {"cas",             no_argument,       NULL, 'k'},
    {"delegate",             no_argument,       NULL, 'd'},
    {"racy_with_delegate",             no_argument,       NULL, 'm'},

    {"verbose",             no_argument,       NULL, 'v'},
    {"help",             no_argument,       NULL, 'h'},
    {NULL,               no_argument,       NULL, 0}
  };
  int c, option_index = 1;
  while ((c = getopt_long(argc, argv, "c:i:abxyefgjkdmnopvh?",
			  long_options, &option_index)) >= 0) {
    switch (c) {
    case 0:   // flag set
      break;
    case 'c': num_cores = atoi(optarg); break;
    case 'i': iters = 1 << atoi(optarg); break;
    case 'v': human_readable = 1; break;

    case 'a': racy_single = 1; break;
    case 'b': racy_double = 1; break;
    case 'x': racy_store_fence = 1; break;
    case 'y': unrelated_full_fence = 1; break;
    case 'e': racy_full_fence = 1; break;
    case 'f': racy_atomic = 1; break;
    case 'g': unrelated_atomic = 1; break;
    case 'j': atomic = 1; break;
    case 'k': cas = 1; break;
    case 'd': delegate = 1; break;
    case 'm': racy_with_delegate = 1; break;
    case 'n': atomic_unrelated = 1; break;
    case 'o': unrelated = 1; break;
    case 'p': atomic_unrelated_load = 1; break;

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


  // unrelated
  if (unrelated) {
    uint64_t counter = 0;
    uint64_t* counter_p = &counter;
    struct timespec start, end;
    uint64_t sum = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);
#pragma omp parallel for num_threads(num_cores) reduction(+ : sum)
    for( int i = 0; i < num_cores; ++i ) {

      setaffinity(i);
      uint64_t data = 0;
      uint64_t local[0x10000] = {0};
      local[0] = counter;
      uint64_t max = iters;
      while ( max-- ) {
	//__sync_fetch_and_add( &data, 1 );
	//data++;
	//argh
	local[max & 0xff]++;
	//sum += val;
      }
      sum += local[0];
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    uint64_t runtime_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec) - ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);

    result(human_readable, num_cores, iters, "unrelated", runtime_ns, counter, sum);

  }

  // racy single
  if (racy_single) {

    uint64_t counter = 0;
    uint64_t* counter_p = &counter;
    struct timespec start, end;
    uint64_t global[0x10000] = {0};
    uint64_t sum = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);
#pragma omp parallel for num_threads(num_cores) reduction(+ : sum)
    for( int i = 0; i < num_cores; ++i ) {
      
      setaffinity(i);
    
      uint64_t local[0x10000] = {0};
      //uint64_t local;
      uint64_t index = 0;

      uint64_t data = 0;
      uint64_t max = iters;
      while( max-- ) {
	global[max & 0xff]++;
      }
      sum += global[0];
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    uint64_t runtime_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec) - ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);

    result(human_readable, num_cores, iters, "racy", runtime_ns, counter, sum);
  }

  

  // racy double
  if (racy_double) {
    uint64_t counter = 0;
    uint64_t* counter_p = &counter;
    struct timespec start, end;
    uint64_t sum = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);
#pragma omp parallel for num_threads(num_cores) reduction(+ : sum)
    for( int i = 0; i < num_cores; ++i ) {
      
      setaffinity(i);
    
      uint64_t local[0x10000] = {0};
      //uint64_t local;
      uint64_t index = 0;

      uint64_t data = 0;
      uint64_t max = num_cores * iters;
      while ( data < iters ) {
      	(*counter_p)++;
	data++;
	local[index & 0xffff]++;
	//__sync_fetch_and_add( &local[index & 0xffff] , 1 );
	index += 8;
	//__sync_fetch_and_add( &local , 1 );
	//local+=1;
      	//sum += val;
      }
      sum += data;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    uint64_t runtime_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec) - ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);

    result(human_readable, num_cores, iters, "racy double", runtime_ns, counter, sum);
  }


  // racy with store fence
  if (racy_store_fence) {
    uint64_t counter = 0;
    uint64_t* counter_p = &counter;
    struct timespec start, end;
    uint64_t sum = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);
#pragma omp parallel for num_threads(num_cores) reduction(+ : sum)
    for( int i = 0; i < num_cores; ++i ) {
      
      setaffinity(i);

      uint64_t local[0x10000] = {0};
      //uint64_t local;
      uint64_t index = 0;

      uint64_t data = 0;
      uint64_t max = num_cores * iters;
      while ( data < iters ) {
	(*counter_p)++;
	data++;
	local[index & 0xffff]++;
	__builtin_ia32_sfence();
	//__sync_fetch_and_add( &local[index & 0xffff] , 1 );
	index += 8;
	//__sync_fetch_and_add( &local , 1 );
	//local+=1;
      	//sum += val;
      }
      sum += data;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    uint64_t runtime_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec) - ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);

    result(human_readable, num_cores, iters, "racy with store fence", runtime_ns, counter, sum);
  }

  // unrelated with store fence
  if (unrelated_full_fence) {
    uint64_t counter = 0;
    uint64_t* counter_p = &counter;
    struct timespec start, end;
    uint64_t sum = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);
#pragma omp parallel for num_threads(num_cores) reduction(+ : sum)
    for( int i = 0; i < num_cores; ++i ) {
      
      setaffinity(i);

      uint64_t local[0x10000] = {0};
      //uint64_t local;
      uint64_t index = 0;

      uint64_t data = 0;
      uint64_t max = num_cores * iters;
      while ( data < iters ) {
	//local[index & 0xffff]++;
	//index += 8;
	data++;
      	//data = (*counter_p)++;
	local[index & 0xffff]++;
	__builtin_ia32_sfence();
	//__sync_fetch_and_add( &local[index & 0xffff] , 1 );
	index += 8;
	//__sync_fetch_and_add( &local , 1 );
	//local+=1;
      	//sum += val;
      }
      sum += data;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    uint64_t runtime_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec) - ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);

    result(human_readable, num_cores, iters, "unrelated  with store fence", runtime_ns, counter, sum);
  }


  // racy full fence
  if (racy_full_fence) {
    uint64_t counter = 0;
    uint64_t* counter_p = &counter;
    struct timespec start, end;
    uint64_t sum = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);
#pragma omp parallel for num_threads(num_cores) reduction(+ : sum)
    for( int i = 0; i < num_cores; ++i ) {

      setaffinity(i);

      uint64_t local[0x10000] = {0};
      //uint64_t local;
      uint64_t index = 0;

      uint64_t data = 0;
      uint64_t max = num_cores * iters;
      while ( data < iters ) {
      	(*counter_p)++;
	data++;
	local[index & 0xffff]++;
	__builtin_ia32_mfence();
	//__sync_fetch_and_add( &local[index & 0xffff] , 1 );
	index += 8;
	//__sync_fetch_and_add( &local , 1 );
	//local+=1;
      	//sum += val;
      }
      sum += data;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    uint64_t runtime_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec) - ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);

    result(human_readable, num_cores, iters, "racy with full fence", runtime_ns, counter, sum);
  }


  // racy with atomic
  if (racy_atomic) {
    uint64_t counter = 0;
    uint64_t* counter_p = &counter;
    struct timespec start, end;
    uint64_t sum = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);
#pragma omp parallel for num_threads(num_cores) reduction(+ : sum)
    for( int i = 0; i < num_cores; ++i ) {
      
      setaffinity(i);

      uint64_t local[0x10000] = {0};
      //uint64_t local;
      uint64_t index = 0;

      uint64_t data = 0;
      uint64_t max = num_cores * iters;
      while ( data < iters ) {
	(*counter_p)++;
	data++;
	__sync_fetch_and_add( &local[index & 0xffff] , 1 );
	index += 8;
	//__sync_fetch_and_add( &local , 1 );
	//local+=1;
      	//sum += val;
      }
      sum += data;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    uint64_t runtime_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec) - ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);

    result(human_readable, num_cores, iters, "racy with atomic", runtime_ns, counter, sum);
  }


  // unrelated with atomic
  if (unrelated_atomic) {
    uint64_t counter = 0;
    uint64_t* counter_p = &counter;
    struct timespec start, end;
    uint64_t sum = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);
#pragma omp parallel for num_threads(num_cores) reduction(+ : sum)
    for( int i = 0; i < num_cores; ++i ) {
      
      setaffinity(i);

      uint64_t local[0x10000] = {0};
      //uint64_t local;
      uint64_t index = 0;

      uint64_t data = 0;
      uint64_t max = num_cores * iters;
      while ( data < iters ) {
      	data++; // = (*counter_p)++;
	__sync_fetch_and_add( &local[index & 0xffff] , 1 );
	index += 8;
	//__sync_fetch_and_add( &local , 1 );
	//local+=1;
      	//sum += val;
      }
      sum += data;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    uint64_t runtime_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec) - ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);

    result(human_readable, num_cores, iters, "unrelated with atomic", runtime_ns, counter, sum);
  }

  // atomic only
  if (atomic) {
    uint64_t counter = 0;
    uint64_t* counter_p = &counter;
    struct timespec start, end;
    uint64_t sum = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);
#pragma omp parallel for num_threads(num_cores) reduction(+ : sum)
    for( int i = 0; i < num_cores; ++i ) {

      setaffinity(i);

      uint64_t max = iters;
      while ( max-- ) {
	uint64_t val = __sync_fetch_and_add( counter_p, 1 );
	//sum += val;
      }
      sum += iters;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    uint64_t runtime_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec) - ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);

    result(human_readable, num_cores, iters, "atomic", runtime_ns, counter, sum);

  }

  // atomic unrelated
  if (atomic_unrelated) {
    uint64_t counter = 0;
    uint64_t* counter_p = &counter;
    struct timespec start, end;
    uint64_t sum = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);
#pragma omp parallel for num_threads(num_cores) reduction(+ : sum)
    for( int i = 0; i < num_cores; ++i ) {

      setaffinity(i);
      uint64_t data = 0;
      uint64_t max = iters;
      while ( max-- ) {
	__sync_fetch_and_add( &data, 1 );
	//sum += val;
      }
      sum += iters;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    uint64_t runtime_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec) - ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);

    result(human_readable, num_cores, iters, "atomic unrelated", runtime_ns, counter, sum);

  }

  // atomic unrelated load
  if (atomic_unrelated_load) {
    uint64_t counter = 0;
    uint64_t* counter_p = &counter;
    struct timespec start, end;
    uint64_t global[0x10000] = {0};
    uint64_t sum = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);
#pragma omp parallel for num_threads(num_cores) reduction(+ : sum)
    for( int i = 0; i < num_cores; ++i ) {

      setaffinity(i);
      uint64_t data = 0;
      uint64_t max = iters;
      while ( max-- ) {
	int val = global[max & 0xff];
	__sync_fetch_and_add( &data, val );
	//sum += val;
      }
      sum += iters;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    uint64_t runtime_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec) - ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);

    result(human_readable, num_cores, iters, "atomic unrelated load", runtime_ns, counter, sum);

  }



  // cas
  if (cas) {
    uint64_t counter = 0;
    uint64_t* counter_p = &counter;
    struct timespec start, end;
    uint64_t sum = 0;

    
    clock_gettime(CLOCK_MONOTONIC, &start);
#pragma omp parallel for num_threads(num_cores) reduction(+ : sum)
    for( int i = 0; i < num_cores; ++i ) {

      setaffinity(i);

      uint64_t data = 0;
      uint64_t max = num_cores * iters;
      while( data < max ) {
	//data = *counter_p;
	data = __sync_val_compare_and_swap(counter_p, data, data + 1);
	//sum += data;
      }
      sum += data;
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    uint64_t runtime_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec) - ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);

    result(human_readable, num_cores, iters, "cas", runtime_ns, counter, sum);

  }





  // delegate
  if (delegate) {
    uint64_t counter = 1;
    uint64_t* counter_p = &counter;
    struct timespec start, end;
    uint64_t sum = 0;


    SW_Queue to[12];

    for(int i = 0; i < 12; ++i) {
      to[i] = sq_createQueue();
    }
      
    int done = 0;

    SW_Queue asdf = sq_createQueue();

    clock_gettime(CLOCK_MONOTONIC, &start);
    
# pragma omp parallel for num_threads(num_cores+1) 
    for(int i = 0; i <= num_cores; ++i) {
      if (i == num_cores) {
	
	#pragma omp task 
	{
	  char str[1024];
	  cpu_set_t set;
	  CPU_ZERO(&set);
	  if (i < 6) {
	    CPU_SET(i*2,&set);
	  } else {
	    CPU_SET(12,&set);
	  }
	  sched_setaffinity(0, sizeof(cpu_set_t), &set);
	  CPU_ZERO(&set);
	  sched_getaffinity(0, sizeof(cpu_set_t), &set);
	  int j; char* c;
	  // for(j = 0, c = str; j < 64; j++, c++) {
	  //   sprintf(c, "%d", CPU_ISSET(j, &set));
	  // }
	  // printf("%s %02d: %s\n", "Consumer", i, str);


	  thread * master = thread_init();
	  scheduler * sched = create_scheduler(master);
	  thread * threads[12] = { NULL };
	  struct consumer_args args[12];
	  for (int i = 0; i < num_cores; ++i) {
	    args[i].max = iters;
	    args[i].q = to[i];
	    threads[i] = thread_spawn(master, sched, consumer, &args[i]);
	  }
	  run_all(sched);
	  printf("Thread %d done consuming.\n", i);
	  destroy_scheduler(sched);
	  destroy_thread(master);
	}
      } else {

	#pragma omp task 
	{
	  if (0) {
	    setaffinity(i);
	    
	    uint64_t data = 0;
	    for(data = 0; data < iters; ++data) {
	      //sq_produce(to[i], (uint64_t) &counter);
	    }
	    sq_flushQueue(to[i]);
	    //sum += data;
	    //std::cout << "Thread " << i << " done producing." << std::endl;
	    printf("Thread %d done producing.\n", i);
	  } else {
	    thread * master = thread_init();
	    scheduler * sched = create_scheduler(master);
	    struct consumer_args args;
	    args.max = iters;
	    args.q = to[i];
	    args.counter = (uint64_t) &counter;
	    thread_spawn(master, sched, producer, &args);
	    run_all(sched);
	    printf("Producer %d done.\n",i);
	    destroy_scheduler(sched);
	    destroy_thread(master);
	  }
	}
	

      }
    }
   
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    uint64_t runtime_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec) - ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);

    result(human_readable, num_cores, iters, "delegate", runtime_ns, counter, sum);

  }


//   // racy with delegate
//   if (racy_with_delegate) {
//     uint64_t counter = 1;
//     uint64_t* counter_p = &counter;
//     struct timespec start, end;
//     uint64_t sum = 0;

    
//     const int size = 1024;
//     ff::SWSR_Ptr_Buffer* to[12] = {NULL};

//     for(int i = 0; i < 12; ++i) {
//       to[i] = new ff::SWSR_Ptr_Buffer(size);
//       to[i]->init();
//     }
      
//     int done = 0;

    

//     clock_gettime(CLOCK_MONOTONIC, &start);
    
// # pragma omp parallel for num_threads(num_cores) 
//     for(int i = 0; i <= num_cores; ++i) {
//       if (i == num_cores) {
	
// 	#pragma omp task 
// 	{
// 	  char str[1024];
// 	  cpu_set_t set;
// 	  CPU_ZERO(&set);
// 	  if (i < 6) {
// 	    CPU_SET(i*2,&set);
// 	  } else {
// 	    CPU_SET(12,&set);
// 	  }
// 	  sched_setaffinity(0, sizeof(cpu_set_t), &set);
// 	  CPU_ZERO(&set);
// 	  sched_getaffinity(0, sizeof(cpu_set_t), &set);
// 	  int j; char* c;
// 	  // for(j = 0, c = str; j < 64; j++, c++) {
// 	  //   sprintf(c, "%d", CPU_ISSET(j, &set));
// 	  // }
// 	  // printf("%s %02d: %s\n", "Consumer", i, str);

// 	  uint64_t data = 0;
// 	  uint64_t max = num_cores * iters;
// 	  void* p;
// 	  while( data < max ) {
// 	    // if (to[1]->pop(&p)) data++;
// 	    // if (to[2]->pop(&p)) data++;
// 	    // if (to[3]->pop(&p)) data++;
// 	    // if (to[4]->pop(&p)) data++;
// 	    // if (to[5]->pop(&p)) data++;
// 	    for(int i = 0; i < num_cores; i++) {
// 	      data += to[i]->pop(&p);
// 	      //if (to[i]->pop(&p)) data++;
// 	    }
// 	    // data += to[0]->pop(&p);
// 	    // data += to[1]->pop(&p);
// 	    // data += to[2]->pop(&p);
// 	    // data += to[3]->pop(&p);
// 	    // data += to[4]->pop(&p);
// 	    // data += to[5]->pop(&p);
// 	    // data += to[6]->pop(&p);
// 	    // data += to[7]->pop(&p);
// 	    // data += to[8]->pop(&p);
// 	    // data += to[9]->pop(&p);
// 	    // data += to[10]->pop(&p);
// 	    // data += to[11]->pop(&p);
// 	  }

// 	}

//       } else {

// 	#pragma omp task 
// 	{
// 	  char str[1024];
// 	  cpu_set_t set;
// 	  CPU_ZERO(&set);
// 	  if (i < 6) {
// 	    CPU_SET(i*2,&set);
// 	  } else {
// 	    CPU_SET((i-6)*2+1,&set);
// 	  }
// 	  sched_setaffinity(0, sizeof(cpu_set_t), &set);
// 	  CPU_ZERO(&set);
// 	  sched_getaffinity(0, sizeof(cpu_set_t), &set);
// 	  int j; char* c;
// 	  // for(j = 0, c = str; j < 64; j++, c++) {
// 	  //   sprintf(c, "%d", CPU_ISSET(j, &set));
// 	  // }
// 	  // printf("%s %02d: %s\n", "Producer", i, str);

// 	  uint64_t data = 0;
// 	  //	  uint64_t data2 = 0;
// 	  while( data < iters) {
// 	    //if (to[i]->push((void*) 1)) data++;
// 	    data += to[i]->push((void*) 1);
// 	    //	    data2++;
// 	  }
// 	  sum += data;
// 	}
	

//       }
//     }

//     clock_gettime(CLOCK_MONOTONIC, &end);
    
//     uint64_t runtime_ns = ((uint64_t) end.tv_sec * 1000000000 + end.tv_nsec) - ((uint64_t) start.tv_sec * 1000000000 + start.tv_nsec);

//     result(human_readable, num_cores, iters, "racy with delegate", runtime_ns, counter, sum);

//   }

  /* if (human_readable) */
  /*   printf("num_cores, iters, " */
  /* 	   "racy_runtime_ns, racy_runtime_per_iter, racy_runtime_per_inc, racy_throughput, racy_result, racy_sum, " */
  /* 	   "atomic_runtime_ns, atomic_runtime_per_iter, atomic_runtime_per_inc, atomic_throughput, atomic_result, atomic_sum, " */
  /* 	   "cas_runtime_ns, cas_runtime_per_iter, cas_runtime_per_inc, cas_throughput, cas_result, cas_sum\n"); */
  /* printf("%g, %g, " */
  /* 	 "%g, %g, %g, %g, %g, %g, " */
  /* 	 "%g, %g, %g, %g, %g, %g, " */
  /* 	 "%g, %g, %g, %g, %g, %g\n",  */
  /* 	 (double)num_cores, (double)iters,  */
  /*   (double)racy_runtime_ns, racy_runtime_per_iter, racy_runtime_per_inc, 1000000000.0/racy_runtime_per_inc, (double)racy_result, (double)racy_sum,  */
  /*   (double)atomic_runtime_ns, atomic_runtime_per_iter, atomic_runtime_per_inc, 1000000000.0/atomic_runtime_per_inc, (double)atomic_result, (double)atomic_sum,  */
  /*   (double)cas_runtime_ns, cas_runtime_per_iter, cas_runtime_per_inc, 1000000000.0/cas_runtime_per_inc, (double)cas_result, (double)cas_sum); */
}
