#include <features.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <getopt.h>
#include <omp.h>

#include <sched.h>
#include <hugetlbfs.h>

#include <sw_queue_greenery.h>
#include <thread.h>

#define MILLION 1000000
#define BILLION 1000000000

uint64_t* getIndices(uint64_t fieldsize, uint64_t num, int isRandom, int num_threads);
uint64_t get_timediff_ns(struct timespec* start, struct timespec* end);
void processArgs(int argc, char** argv, uint64_t* fieldsize, uint64_t* num_ups, int* num_threads, int* isRandom, int* isAtomic, int* isDelegated);

typedef struct consumer_args {
  SW_Queue q;
  uint64_t max;
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



int main(int argc, char** argv) {
    uint64_t fieldsize = 1<<30;
    uint64_t num_ups_percore = 1<<24;
    int isRandom = 1;
    int num_threads = 1;
    int isAtomic = 0;
    int isDelegated = 0;

    processArgs(argc, argv, &fieldsize, &num_ups_percore, &num_threads, &isRandom, &isAtomic, &isDelegated);
    uint64_t num_ups = num_ups_percore*num_threads;

   //printf("%lu fieldsize\n",fieldsize*sizeof(uint64_t)); 
    uint64_t* field = (uint64_t*)malloc(fieldsize*sizeof(uint64_t));
    //int64_t min_field_alloc = (1 << 30) * (1 + fieldsize*sizeof(uint64_t) / (1 << 30));
    //uint64_t* field = get_huge_pages(min_field_alloc, GHP_DEFAULT);
    if (field==0) {
        printf("malloc failed\n");
        exit(1);
    }
    printf("allocated %lu GB\n",fieldsize*sizeof(uint64_t)/BILLION);
    //printf("field ptr=%lx\n",field);
    printf("done field\n");
    uint64_t* indices = getIndices(fieldsize, num_ups,isRandom,num_threads);
    printf("done ind\n");


    SW_Queue to[12];
    
    for(int i = 0; i < 12; ++i) {
      to[i] = sq_createQueue();
    }


    struct timespec start;
    struct timespec end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    
    if (isDelegated) {
      uint64_t num_cores = num_threads;
# pragma omp parallel for num_threads(num_cores+1) 
      for(int core = 0; core <= num_cores; ++core) {
	if (core == num_cores) {

	  // consumer
#pragma omp task 
	  {
	    cpu_set_t set;
	    CPU_ZERO(&set);
	    if (core < 6) {
	      CPU_SET(core*2,&set);
	    } else {
	      CPU_SET(12,&set);
	    }
	    sched_setaffinity(0, sizeof(cpu_set_t), &set);
	    
	    uint64_t max = num_ups / num_cores;
	    thread * master = thread_init();
	    scheduler * sched = create_scheduler(master);
	    thread * threads[12] = { NULL };
	    struct consumer_args args[12];
	    for (int i = 0; i < num_cores; ++i) {
	      args[i].max = max;
	      args[i].q = to[i];
	      threads[i] = thread_spawn(master, sched, consumer, &args[i]);
	    }
	    run_all(sched);
	    //destroy_scheduler(sched);
	    //destroy_thread(master);
	  }
	} else {
	  
	  // producer
#pragma omp task 
	  {
	    cpu_set_t set;
	    CPU_ZERO(&set);
	    if (core < 6) {
	      CPU_SET(core*2,&set);
	    } else {
	      CPU_SET((core-6)*2+1,&set);
	    }
	    sched_setaffinity(0, sizeof(cpu_set_t), &set);

        uint64_t num_ups_percore = num_ups/num_cores;
        uint64_t endcond = (core+1)*num_ups_percore;
	    for(uint64_t i=core*num_ups_percore; i<endcond; ++i) { // partitioning indices manually  
          sq_produce(to[core], (uint64_t) &field[indices[i]]);
	    }
	    sq_flushQueue(to[core]);
	  }
	}
      }
    } else if (isAtomic) {
        #pragma omp parallel num_threads(num_threads)
        {
         #pragma omp for
         for (uint64_t i=0; i<num_ups; i++) {             // omp will give num_ups_percore to each thread
             __sync_fetch_and_add(&field[indices[i]], 1);
         }
        }
    } else {
         #pragma omp parallel num_threads(num_threads)
        {
         #pragma omp for
         for (uint64_t i=0; i<num_ups; i++) {            // omp will give num_ups_percore to each thread
             field[indices[i]]++;
         }
        }
    }
        
    
    
    clock_gettime(CLOCK_MONOTONIC, &end);

    uint64_t runtime_ns = get_timediff_ns(&start, &end);

    float gups = ((float)num_ups)/(runtime_ns);
    float mbperitem = sizeof(uint64_t)/((float)MILLION);
    float gupdates = num_ups/((float)BILLION);
    float ns_per_up = ((float)runtime_ns)/num_ups;
    float mb_rate = gups*BILLION*mbperitem;
printf("%ld fieldsize, %f Gupdates, %ld ns, %f ns/up, %f gups, %f MB/s\n", fieldsize, gupdates, runtime_ns, ns_per_up, gups, mb_rate);

 printf("{'fieldsize':%ld, 'updates':%ld, 'gups':%f, 'runtime_ns':%ld, 'atomic':%d, 'random':%d, 'num_threads':%d, 'f_ele_size':%lu, 'delegated':%d}\n", fieldsize, num_ups, gups, runtime_ns, isAtomic, isRandom, num_threads, sizeof(uint64_t), isDelegated);

    return 0;
}

uint64_t* getIndices(uint64_t fieldsize, uint64_t num, int isRandom, int num_threads) {

  uint64_t* indices = (uint64_t*)malloc(num*sizeof(uint64_t));
  //int64_t min_indices_alloc = (1 << 30) * (1 + num*sizeof(uint64_t) / (1 << 30));
  //uint64_t* indices = get_huge_pages(min_indices_alloc, GHP_DEFAULT);

    if (isRandom) { 
        unsigned int seedp;
        srand((unsigned int)time(NULL));
        #pragma omp parallel num_threads(num_threads) private(seedp)
        {
            seedp = omp_get_thread_num();
            #pragma omp for
            for (uint64_t i=0; i<num; i++) {
                indices[i] = rand_r(&seedp) / (RAND_MAX/fieldsize + 1);
                //printf("%lu indices[%lu]\n",indices[i],i);
            }
        }
    } else {
        #pragma omp parallel num_threads(num_threads)
        {
        #pragma omp for
        for (uint64_t i=0; i<num; i++) {
            indices[i] = i%fieldsize;
        }
        }
    }

    return indices;
}

uint64_t get_timediff_ns(struct timespec* start, struct timespec* end) {
    return ((uint64_t) end->tv_sec * BILLION + end->tv_nsec) - ((uint64_t) start->tv_sec * BILLION + start->tv_nsec);
}



void processArgs(int argc, char** argv, uint64_t* fieldsize, uint64_t* num_ups, int* num_threads, int* isRandom, int* isAtomic, int* isDelegated) {


  static struct option long_options[] = {
    {"fieldsize",             required_argument, NULL, 'f'},
    {"updates",               required_argument, NULL, 'u'},
    {"cores",            required_argument, NULL, 'c'},
    {"no-random",        no_argument,       NULL, 'n'},
    {"atomic",           no_argument,       NULL, 'a'},
    {"delegate",         no_argument,       NULL, 'd'},
    {"help",             no_argument,       NULL, 'h'},
    {NULL,               no_argument,       NULL, 0}
  };
  int c, option_index = 1;
  while ((c = getopt_long(argc, argv, "f:u:c:nadh?",
                          long_options, &option_index)) >= 0) {
    switch (c) {
    case 0:   // flag set
      break;
    case 'f':
      *fieldsize = (1L<<(atoi(optarg)));
      break;
    case 'u':
      *num_ups = (1L<<(atoi(optarg)));
      break;
    case 'c':
      *num_threads = atoi(optarg);
      break;
    case 'n':
        *isRandom = 0;
        break;
    case 'a':
        *isAtomic = 1;
        break;
    case 'd':
      *isDelegated = 1;
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
}
