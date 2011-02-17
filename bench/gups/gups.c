#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <getopt.h>

uint32_t* getIndices(int fieldsize, int num, int isRandom);
uint64_t get_timediff_ns(struct timespec* start, struct timespec* end);
void processArgs(int argc, char** argv, int* fieldsize, int* num_ups, int* num_threads, int* isRandom, int* isAtomic);



int main(int argc, char** argv) {
    int fieldsize = 1<<30;
    int num_ups = 1<<24;
    int isRandom = 1;
    int num_threads = 1;
    int isAtomic = 0;

    processArgs(argc, argv, &fieldsize, &num_ups, &num_threads, &isRandom, &isAtomic);

    uint64_t* field = (uint64_t*)malloc(fieldsize*sizeof(uint64_t));
    uint32_t* indices = getIndices(fieldsize, num_ups,isRandom);


    struct timespec start;
    struct timespec end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    
    if (isAtomic) {
        #pragma omp parallel num_threads(num_threads)
        {
         #pragma omp for
         for (int i=0; i<num_ups; i++) {
             __sync_fetch_and_add(&field[indices[i]], 1);
         }
        }
    } else {
         #pragma omp parallel num_threads(num_threads)
        {
         #pragma omp for
         for (int i=0; i<num_ups; i++) {
             field[indices[i]]++;
         }
        }
    }
        
    
    
    clock_gettime(CLOCK_MONOTONIC, &end);

    uint64_t runtime_ns = get_timediff_ns(&start, &end);

    int MILLION=1000000;
    int BILLION=1000000000;
    float gups = ((float)num_ups)/(runtime_ns);
    float mbperitem = sizeof(uint64_t)/((float)MILLION);
printf("%d fieldsize, %f Gupdates, %ld ns, %f ns/up, %f gups, %f MB/s\n", fieldsize, num_ups/((float)BILLION), runtime_ns, ((float)runtime_ns)/num_ups, gups, gups*BILLION*mbperitem);


    return 0;
}

uint32_t* getIndices(int fieldsize, int num, int isRandom) {
    srand ((unsigned int) time (NULL));

    uint32_t* indices = (uint32_t*)malloc(num*sizeof(uint32_t));

    if (isRandom) {
        for (int i=0; i<num; i++) {
            indices[i] = rand()%fieldsize;
        }
    } else {
        for (int i=0; i<num; i++) {
            indices[i] = i;
        }
    }

    return indices;
}

uint64_t get_timediff_ns(struct timespec* start, struct timespec* end) {
    int BILLION = 1000000000;
    return ((uint64_t) end->tv_sec * BILLION + end->tv_nsec) - ((uint64_t) start->tv_sec * BILLION + start->tv_nsec);
}



void processArgs(int argc, char** argv, int* fieldsize, int* num_ups, int* num_threads, int* isRandom, int* isAtomic) {


  static struct option long_options[] = {
    {"fieldsize",             required_argument, NULL, 'f'},
    {"updates",               required_argument, NULL, 'u'},
    {"cores",            required_argument, NULL, 'c'},
    {"no-random",        no_argument,       NULL, 'n'},
    {"atomic",           no_argument,       NULL, 'a'},
    {"help",             no_argument,       NULL, 'h'},
    {NULL,               no_argument,       NULL, 0}
  };
  int c, option_index = 1;
  while ((c = getopt_long(argc, argv, "f:u:c:nah?",
                          long_options, &option_index)) >= 0) {
    switch (c) {
    case 0:   // flag set
      break;
    case 'f':
      *fieldsize = (1<<(atoi(optarg)));
      break;
    case 'u':
      *num_ups = (1<<(atoi(optarg)));
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
