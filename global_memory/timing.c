#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "timing.h"

#define rdtscll(val) do { \
  unsigned int __a,__d; \
  asm volatile("rdtsc" : "=a" (__a), "=d" (__d)); \
  (val) = ((unsigned long)__a) | (((unsigned long)__d)<<32); \
  } while(0)


Timer* timing_createSimpleTimer() {
    Timer* t = timing_createTimer(2, 0);
    t->doBin = false;
    return t;
}

Timer* timing_createTimer(uint64_t num_bins, uint64_t bin_max) {
    Timer* t = (Timer*) malloc(sizeof(Timer));
    t->last_time = 0;
    t->number_of_samples = 0;
    t->max_sample = 0;
    t->difference_sum = 0;
    t->num_bins = num_bins;
    t->bin_max = bin_max;

    t->bins = (uint64_t*)malloc(sizeof(uint64_t)*num_bins);
    bzero(t->bins, sizeof(uint64_t)*num_bins);
    
    t->bin_width = bin_max/(num_bins-1);
    t->doBin = true;
    return t;
}

void timing_markTime(Timer* t) {
    unsigned int current;
    rdtscll(current);
    
    // first call
    if (t->last_time == 0) {
        t->last_time = current;
        return;
    }

    uint64_t diff = current - t->last_time;
    t->max_sample = (diff > t->max_sample) ? diff : t->max_sample;
    t->last_time = current;
    t->number_of_samples++;
    t->difference_sum += diff;

    if (t->doBin) {
        if (diff >= t->bin_max) {
            t->bins[t->num_bins-1]++; 
         } else {
            uint64_t index = diff / t->bin_width;
            t->bins[index]++;
        }
    }
}

double timing_avgIntervalNs(Timer* t) {
    return ((double)t->difference_sum / t->number_of_samples)/NS_PER_TICK;
}

void timing_printHistogram(Timer* t) {
    uint64_t num_bins = t->num_bins;
    uint64_t bin_width = t->bin_width/NS_PER_TICK;
    uint64_t* bins = t->bins;
    printf("histogram: (ns)\n");
    for (uint64_t i = 0; i < num_bins-1; i++) {
        printf("[%lu,%lu):%lu\n", i*bin_width, (i+1)*bin_width, bins[i]);
    }
    printf("[%lu,%lu):%lu\n", t->bin_max, t->max_sample, bins[num_bins-1]);
   
}
