#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sw_queue_astream.h>
#include <omp.h>
#include <time.h>

#define BILLION 1000000000


#define rdtscll(val) do { \
  unsigned int __a,__d; \
  asm volatile("rdtsc" : "=a" (__a), "=d" (__d)); \
  (val) = ((unsigned long)__a) | (((unsigned long)__d)<<32); \
  } while(0)


// a call twice to clock_gettime wastes about 65ns

uint64_t get_time_ns(struct timespec* time) {
    return (uint64_t) time->tv_sec* BILLION + time->tv_nsec;
}

uint64_t get_timediff_ns(struct timespec* start, struct timespec* end) {
    return get_time_ns(end) - get_time_ns(start);
}


int main(int argc, char** argv) {

    if (argc < 4) {
        printf("usage: %s REQ-PER-PAIR CORES BUFSIZE\n", argv[0]);
        exit(1);
    }
    uint64_t num = atoi(argv[1]);
    uint64_t num_cores = atoi(argv[2]);
    uint64_t bufsize = atoi(argv[3]);
    if (num_cores%2!=0) {
        printf("cores must be even to have pairs\n");
        exit(1);
    }
    if (bufsize > CHUNK_SIZE) {
        printf("bufsize > CHUNK_SIZE=%d will not help\n", CHUNK_SIZE);
        exit(1);
    }
    if (bufsize < 1) {
        printf("bufsize must be at least 1\n");
        exit(1);
    }
    
    uint64_t num_qs = num_cores>>1;

    SW_Queue to[num_qs];
    SW_Queue from[num_qs];

    for (int i=0; i<num_qs; i++) {
        to[i] = sq_createQueue();
        from[i] = sq_createQueue();
    }

    uint64_t total_time[12] = {0};
    
    struct timespec start;
    struct timespec end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    #pragma omp parallel num_threads(num_cores)
    {
    #pragma omp for
    for (int i=0; i<num_cores; i++) {
        if (i%2==0) {
            int ind = i/2;
            uint64_t pro = 0;
            uint64_t con = 0;
            //while (j<num) {
            while (con<num) {  
                for (uint64_t k=0; k<bufsize; k++) {

                 //   struct timespec timestamp;
                 //   clock_gettime(CLOCK_MONOTONIC, &timestamp);

                 //   uint64_t timestamp_ns = get_time_ns(&timestamp);
                    uint64_t timestamp_ticks;
                    rdtscll(timestamp_ticks);                   
                    
                    sq_produce(to[ind], timestamp_ticks/*pro+k*/);
                }
                sq_flushQueue(to[ind]);
                pro+=bufsize;
                //printf("produced j=%lu\n",j);
                
                while(sq_canConsume(from[ind])) { 
                    uint64_t resp = sq_consume(from[ind]);
                    con++;
//                    printf("resp%lu\n",resp);
                    
                    //struct timespec currenttime;
                    //clock_gettime(CLOCK_MONOTONIC, &currenttime);
                    //uint64_t currenttime_ns = get_time_ns(&currenttime);
                    uint64_t currenttime_ticks;
                    rdtscll(currenttime_ticks);
                    total_time[i/2] += (currenttime_ticks - resp);
                }
            }
        } else {
            int ind = i/2;
            uint64_t j = 0;
            while (j<num) {
                uint64_t count = 0;
                while(sq_canConsume(to[ind])) {
                    uint64_t req = sq_consume(to[ind]);
                    count++;
             //   printf("consumed %lu\n", req);
                    sq_produce(from[ind],/*++*/ req);
                }

               sq_flushQueue(from[ind]);
               j+=count;
            }
        }
    }
    }   
   
    clock_gettime(CLOCK_MONOTONIC, &end);
    uint64_t runtime_ns = get_timediff_ns(&start, &end);
    uint64_t totalnum = num*num_qs;

    // get average latency
    uint64_t total_total_time = 0;
    for (int i=0; i<num_qs; i++) total_total_time += total_time[i];
    double avg_latency_ticks = ((double)total_total_time)/(num_qs * num);

    double avg_latency_ns = avg_latency_ticks/2.66;

    printf("{'rate':%f, 'bufsize':%lu, 'num_cores':%lu, 'runtime_ns':%lu, 'total_refs':%lu, 'refs_per_q':%lu, 'avg_latency_ns':%f}\n", totalnum/((float)runtime_ns)*BILLION, bufsize, num_cores, runtime_ns, totalnum, num, avg_latency_ns);

    return 0;
}
