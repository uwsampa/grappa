#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <string>
#include "timing.hpp"

#define rdtscll(val) do { \
  unsigned int __a,__d; \
  asm volatile("rdtsc" : "=a" (__a), "=d" (__d)); \
  (val) = ((unsigned long)__a) | (((unsigned long)__d)<<32); \
  } while(0)



Timer::Timer(std::string name, uint64_t num_bins, uint64_t bin_max, bool binning)
    : last_time(0)
    , number_of_samples(0)
    , max_sample(0)
    , difference_sum(0)
    , num_bins(num_bins)
    , bin_max(bin_max)
    , bins((uint64_t*)malloc(sizeof(uint64_t)*num_bins))
    , bin_width(bin_max/(num_bins-1))
    , doBin(binning)
    , doPrint(binning)
    , name(name)
    {

        bzero(bins, sizeof(uint64_t)*num_bins);
}

Timer::~Timer() {
    if (doPrint) {
        if (doBin) {
            printHistogram();
        }
        printf("%s: avg interval (ns): %f\n", name.c_str(), avgIntervalNs());
    }

    free (bins);
}

Timer* Timer::createSimpleTimer(std::string name) {
    return new Timer(name, 2, 0, false);
}

Timer* Timer::createTimer(std::string name, uint64_t num_bins, uint64_t bin_max) {
    return new Timer(name, num_bins, bin_max, true);
}


void Timer::markTime(bool accumWithLast) {
    uint64_t current;
    rdtscll(current);
   
    // first call only initializes last_time; also don't accum if false 
    if (accumWithLast && last_time != 0) {
        uint64_t diff = current - last_time;
        max_sample = (diff > max_sample) ? diff : max_sample;
        number_of_samples++;
        difference_sum += diff;

        if (doBin) {
            printf("current:%lu, last:%lu\n", current, last_time);
            if (diff >= bin_max) {
                printf("BAD\n");
                bins[num_bins-1]++; 
             } else {
                uint64_t index = diff / bin_width;
                bins[index]++;
            }
        }
    }

    last_time = current;
}

double Timer::avgIntervalNs() {
    return ((double)difference_sum / number_of_samples)/NS_PER_TICK;
}

void Timer::printHistogram() {
    uint64_t bin_width_ns = bin_width/NS_PER_TICK;
    printf("%s histogram: (ns)\n", name.c_str());
    for (uint64_t i = 0; i < num_bins-1; i++) {
        printf("[%lu,%lu):%lu\n", i*bin_width_ns, (i+1)*bin_width_ns, bins[i]);
    }
    printf("[%lu,%lu):%lu\n", bin_max, max_sample, bins[num_bins-1]);
   
}

void Timer::setPrint(bool on) {
    doPrint = on;
}
