#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <omp.h>

#include "CoreBuffer.hpp"



uint64_t get_time_ns(struct timespec* time) {
    return (uint64_t) time->tv_sec* 1000000000 + time->tv_nsec;
}


uint64_t get_timediff_ns(struct timespec* start, struct timespec* end) {
    return get_time_ns(end) - get_time_ns(start);
}



int main(int argc, char** argv) {
    CoreBuffer* cb = CoreBuffer::createQueue();
    printf("%u\n", cb->canConsume());
    cb->produce(0xDEADBEEF);
    cb->flush();
    printf("%u\n", cb->canConsume());
    uint64_t yum;
    cb->consume(&yum);

    printf("%lx\n", yum);


    #pragma omp parallel for num_threads(2)
    for (int i=0; i<2; i++) {
        if (i==0) {
            cb->produce(0xC0FFEE);
            cb->produce(0xD00DE);
            cb->flush();
            printf("produce %d\n", omp_get_thread_num());
        } else {
            uint64_t yuck;
            cb->consume(&yuck);
            printf("%lx, %d\n", yuck, omp_get_thread_num());
            cb->consume(&yuck);
            printf("%lx\n", yuck);
        }
    }



    struct timespec start;
    struct timespec end;
    uint64_t sum = 0;
    uint64_t iter = 10000000;
    if (argc>1) iter = atoi(argv[1]);

    clock_gettime(CLOCK_MONOTONIC, &start);
     
    #pragma omp parallel for num_threads(2)
    for (int t=0; t<2; t++) {
        if (t==0) {
            for (uint64_t i=0; i<iter; i++) {
 //               printf("%lu\n p", i);
                cb->produce(i);
            }
            cb->flush();
        
        } else {
            for (uint64_t i=0; i<iter; i++) {
                uint64_t yuck;
//                printf("%lu\n", i);
                cb->consume(&yuck);
                sum+=yuck;
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    uint64_t runtime_ns = get_timediff_ns(&start, &end);

    printf("runtime: %lu, rate: %f Mref/s\n", runtime_ns, 1000*((double)iter)/runtime_ns);
    
}
