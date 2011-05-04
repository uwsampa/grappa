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


#define PRODUCE(tt) /*rdtscll((tt));*/ sq_produce(to[ind], (tt)); start_ts[ind][num]=(tt)
#define MIN(A,B) (A<B) ? A : B

// a call twice to clock_gettime wastes about 65ns

uint64_t get_time_ns(struct timespec* time) {
    return (uint64_t) time->tv_sec* BILLION + time->tv_nsec;
}

uint64_t get_timediff_ns(struct timespec* start, struct timespec* end) {
    return get_time_ns(end) - get_time_ns(start);
}


int main(int argc, char** argv) {

    if (argc < 4) {
        printf("usage: %s REQ-PER-PAIR CORES BUFSIZE [latency file]\n", argv[0]);
        exit(1);
    }
    const uint64_t num = atoi(argv[1]);
    const uint64_t num_cores = atoi(argv[2]);
    const uint64_t bufsize = atoi(argv[3]);
    const char* latency_fname = NULL;
    if (argc==5) latency_fname = argv[4];

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
    
    printf("num=%lu, num_cores=%lu, buf=%lu, lat=%s\n",num,num_cores,bufsize,latency_fname);
    const uint64_t num_qs = num_cores>>1;

    SW_Queue to[12];
    
    for (uint64_t i=0; i<num_qs; i++) {
        SW_Queue newq = sq_createQueue();
        to[i] = newq;
    }

    uint64_t total_time[12] = {0};
    uint64_t min_time[12] = {UINT64_MAX};
    uint64_t** end_ts = (uint64_t**)malloc(sizeof(uint64_t*)*num_qs); 
    uint64_t** start_ts = (uint64_t**)malloc(sizeof(uint64_t*)*num_qs); 
    for (int i=0; i<num_qs; i++) {
        end_ts[i] = (uint64_t*)malloc(sizeof(uint64_t)*num);
        start_ts[i] = (uint64_t*)malloc(sizeof(uint64_t)*num);
    }

    printf("starting...\n");

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
            //uint64_t con = 0;
            //while (j<num) {
            while (pro<num) {  
                //for (uint64_t k=0; k<bufsize; k++) {
                 //   struct timespec timestamp;
                 //   clock_gettime(CLOCK_MONOTONIC, &timestamp);

                 //   uint64_t timestamp_ns = get_time_ns(&timestamp);
                    uint64_t timestamp_ticks;
                   rdtscll(timestamp_ticks); 

                   sq_produce(to[ind], timestamp_ticks);
                   start_ts[ind][pro] = timestamp_ticks;
                    
                     
                //}
                //sq_flushQueue(to[ind]);
                pro+=1;//bufsize;
                //printf("produced j=%lu\n",j);
                
            }
            sq_flushQueue(to[ind]);
        } else {
            int ind = i/2;
            uint64_t j = 0;
            while (j<num) {


                uint64_t count = 0;
                    uint64_t req = sq_consume(to[ind]);
                    count++;

                    uint64_t currenttime_ticks;
                    rdtscll(currenttime_ticks);
                    uint64_t diff_ticks = currenttime_ticks - req;
                    total_time[ind] += diff_ticks;
                    min_time[ind] = MIN(diff_ticks, min_time[ind]);
                    end_ts[ind][j] = currenttime_ticks;
             //   printf("consumed %lu\n", req);


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


    // get min latency
    uint64_t min_min_time = UINT64_MAX;
    for (int i=0; i<num_qs; i++) min_min_time = MIN(min_min_time, min_time[i]);
    double min_latency_ns = min_min_time / 2.66;


    // latencies output
    if (latency_fname != NULL) {
        FILE *latency_fp = fopen(latency_fname, "w");
        fprintf(latency_fp, "start, end, queue_id, req_num\n");
        for (int q=0; q<num_qs; q++) {
            for (int i=0; i<num; i++) {
                fprintf(latency_fp, "%f, %f, %d, %d\n", ((double)start_ts[q][i])/2.66, ((double)end_ts[q][i])/2.66, q, i);
            }
        }
        fclose(latency_fp);
    }

    printf("{'rate':%f Mref/s, 'bufsize':%lu, 'num_cores':%lu, 'runtime_ns':%lu, 'total_refs':%lu, 'refs_per_q':%lu, 'avg_latency_ns':%f, 'min_latency_ns':%f}\n", totalnum/((float)runtime_ns)*1000, bufsize, num_cores, runtime_ns, totalnum, num, avg_latency_ns, min_latency_ns);

    return 0;
}
