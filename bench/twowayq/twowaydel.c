#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <omp.h>
#include <time.h>
#include <getopt.h>

#include "CoreBuffer.hpp"


// C++ suppress UINT64_MAX in stdint.h; don't seem to have a library installed that emulates it 
#define UINT64_MAX 0xffffffffffffffff


#define BILLION 1000000000

#ifndef LIMIT_OUTSTANDING
    #define LIMIT_OUTSTANDING 0
#endif


#define IMAX(X, Y) ((X) > (Y) ? (X) : (Y))
#define IMIN(X, Y) ((X) < (Y) ? (X) : (Y))
#define ARR_INIT(ARR,SIZE,VAL) for (int _ind_=0; _ind_<(SIZE); _ind_++) (ARR)[_ind_]=(VAL)

// a call twice to clock_gettime wastes about 65ns

uint64_t get_time_ns(struct timespec* time) {
    return (uint64_t) time->tv_sec* BILLION + time->tv_nsec;
}

uint64_t get_timediff_ns(struct timespec* start, struct timespec* end) {
    return get_time_ns(end) - get_time_ns(start);
}


    
void processArgs(int argc, char** argv, uint64_t* num_reqs, uint64_t* num_cores, uint64_t* bufsize, uint64_t* max_outstanding);


int main(int argc, char** argv) {
printf("LL CC %d\n", LIMIT_OUTSTANDING);
    uint64_t num = 100000;
    uint64_t num_cores = 2;
    uint64_t bufsize = 16;
    uint64_t max_outstanding = 15; // only used if LIMIT_OUTSTANDING
    

    processArgs(argc, argv, &num, &num_cores, &bufsize, &max_outstanding);
    if (bufsize > CHUNK_SIZE) {
        printf("bufsize > CHUNK_SIZE=%d will not help\n", CHUNK_SIZE);
//        exit(1);
    }
    if (bufsize < 1) {
        printf("bufsize must be at least 1\n");
        exit(1);
    }
    if (num_cores < 2) {
        printf("must have at least 2 cores for 1 producer+1 delegate\n");
    }

    uint64_t num_qs = num_cores-1;

    uint64_t totalnum = num*num_qs;

    CoreBuffer* to[num_qs];
    CoreBuffer* from[num_qs];
    
    for (int i=0; i<num_qs; i++) {
        to[i] = CoreBuffer::createQueue();
        from[i] = CoreBuffer::createQueue();
    }


    struct timespec start;
    struct timespec end;

    const int OUTER = 1; // 1 seems to work

    uint64_t result[12] = {0};
    int workersd[12]={0};

    uint64_t total_time[12]; ARR_INIT(total_time,12,0);
    uint64_t max_time[12]; ARR_INIT(max_time,12,0);
    uint64_t min_time[12]; ARR_INIT(min_time,12,UINT64_MAX);
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    #pragma omp parallel num_threads(num_cores)
    {
    #pragma omp for
    for (int i=0; i<num_cores; i++) {
        if (i<num_qs) {
            int ind = i;
            uint64_t pro = 0;
            uint64_t con = 0; 
            uint64_t outstanding = 0;
            while (con<num) {
//                printf("t%d produced: %lu, consumed:%lu\n", ind, pro,con);
                                
                if (pro<num) {
                    uint64_t k = 0;
                    for ( ; k<bufsize; k++) {  // its okay if produce too many
                        struct timespec timestamp;
                        clock_gettime(CLOCK_MONOTONIC, &timestamp);
                        uint64_t timestamp_ns = get_time_ns(&timestamp);
                #if LIMIT_OUTSTANDING
                    if (outstanding < max_outstanding) {
                #endif
     
                    to[ind]->produce(timestamp_ns); 

     
                #if LIMIT_OUTSTANDING
                        outstanding++;
                    } else {
                        break;
                    }
                #endif
                        //printf("w produced %lu %lu\n", pro+k, timestamp_ns);
                    }

                    to[ind]->flush();

                    pro+=k;
                   // outstanding+=k;  // sent k, so k more requests are now outstanding
                   // printf("produced j=%lu\n",pro);
                } 
                
                //while(sq_canConsume(from[ind])) { 
                //    uint64_t resp = sq_consume(from[ind]);
//                    printf("resp%lu\n",resp);
               // }
               uint64_t before = con;
               
               while (from[ind]->canConsume()) {
                 //ANOTHER WAY keep a count of how many pending and don't go over but still need total consumed

                #if LIMIT_OUTSTANDING
                    if (outstanding == 0) break; // seems this should never be true here
                #endif

                   uint64_t val_consumed;
                   from[ind]->consume(&val_consumed);
                   
                   result[ind]+= val_consumed;
                   
                   struct timespec currenttime;
                   clock_gettime(CLOCK_MONOTONIC, &currenttime);
                   uint64_t currenttime_ns = get_time_ns(&currenttime);
                   uint64_t diff_ns = currenttime_ns - val_consumed;
                   total_time[ind] += diff_ns;
                   max_time[ind] = IMAX(diff_ns, max_time[ind]);
                   min_time[ind] = IMIN(diff_ns, min_time[ind]);
                   
                   //printf("w consumed %lu %lu\n", con, val_consumed);

                   con++;
                   outstanding--; // received back a response so 1 fewer requests outstanding
               //printf("outs: %lu\n", outstanding);
               }
//               if (con==before) {sq_produce(to[ind], pro++);sq_flushQueue(to[ind]);/*printf("produced: %lu, consumed:%lu\n", pro,con);*/}
               
            }
            to[ind]->produce(0);
            to[ind]->flush();
            
            printf("finish %d",ind);
        } else if (OUTER) {
            uint64_t jcounts[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
            uint64_t total = 0;
            uint64_t done = 0;
            uint64_t dones[12] =   {0,0,0,0,0,0,0,0,0,0,0,0};
            while (!done) {
                done = 1;
                for (int ind=0;ind<num_qs;ind++) {

                    if (jcounts[ind]<num || to[ind]->canConsume()) {// 2nd clause keeps the last worker from left behind
                   
                        done = 0; // if any counts not done then done=false
                        uint64_t count = 0;
                        
                        while(to[ind]->canConsume() && count<bufsize) { // TODO MCRB can be more efficient with less checks, CoreBuffer reduces flexibility for optimization
                            count++;

                            uint64_t val_consumed;
                            to[ind]->consume(&val_consumed);
                            //printf("consumed %lu %lu\n", jcounts[0]+(count-1), val_consumed);
                            from[ind]->produce(val_consumed);
                            //printf("produced %lu %lu\n", jcounts[0]+(count-1), val_consumed);
                        }
                        from[ind]->flush();
                        
                        jcounts[ind]+=count;
                        total+=count;
//                        printf("jcounts[%d]=%lu of %lu; total=%lu of %lu\n",ind,jcounts[ind], num, total, totalnum);
                    }
                }
            }
            printf("consumer ran away!\n");
        }
    }
    }
    
    
   
    clock_gettime(CLOCK_MONOTONIC, &end);
    uint64_t runtime_ns = get_timediff_ns(&start, &end);
    int num_del = 1;

    // get average latency
    uint64_t total_total_time = 0;
    for (int i=0; i<num_qs; i++) total_total_time += total_time[i];
    double avg_latency = ((double)total_total_time)/(num_qs * num);

    // get max latency
    uint64_t max_max_time = 0;
    for (int i=0; i<num_qs; i++) max_max_time = IMAX(max_time[i], max_max_time);
    
    // get min latency
    uint64_t min_min_time = UINT64_MAX;
    for (int i=0; i<num_qs; i++) min_min_time = IMIN(min_time[i], min_min_time);
      

    for (int i=0; i<12; i++) printf("%lu", result[i]);

    printf("{'rate':%f, 'bufsize':%lu, 'num_workers':%lu, 'runtime_ns':%lu, 'total_refs':%lu, 'refs_per_q':%lu, 'num_del':%d, 'avg_latency_ns':%f, 'max_latency_ns':%lu, 'min_latency_ns':%lu, 'max_outstanding':%lu, 'limit_out':%d}\n", totalnum/((float)runtime_ns)*BILLION, bufsize, num_cores, runtime_ns, totalnum, num, num_del, avg_latency, max_max_time, min_min_time, max_outstanding, LIMIT_OUTSTANDING);

/*struct timespec thing;
struct timespec bling;
clock_gettime(CLOCK_MONOTONIC, &thing);
clock_gettime(CLOCK_MONOTONIC, &bling);
printf("%lu\n", get_timediff_ns(&thing, &bling));
*/
    return 0;
}



void processArgs(int argc, char** argv, uint64_t* num_reqs, uint64_t* num_cores, uint64_t* bufsize, uint64_t* max_outstanding) {
    static struct option long_options[] = {
        {"num-log",         required_argument, NULL, 'n'},
        {"cores",       required_argument, NULL, 'c'},
        {"bufsize",        required_argument,       NULL, 'b'},
        {"max-outstanding",required_argument,       NULL, 'o'},
        {"help",             no_argument,       NULL, 'h'},
        {NULL,               no_argument,       NULL, 0}
    };

    int c, option_index=1;
    while ((c = getopt_long(argc, argv, "n:c:b:o:h?",
                                long_options, &option_index)) >= 0) {
        switch(c) {
            case 0: // flag set
                break;
            case 'n':
                *num_reqs = 1<<(atoi(optarg));
                break;
            case 'c':
                *num_cores = atoi(optarg);
                break;
            case 'b':
                *bufsize = atoi(optarg);
                break;
            case 'o':
                *max_outstanding = atoi(optarg);
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


