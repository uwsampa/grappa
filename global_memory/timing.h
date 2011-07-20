#ifdef __cplusplus
extern "C" {
#endif



#ifndef TIMING_H_
#define TIMING_H_

#define NS_PER_TICK 2.66f

typedef struct Timer {
    uint64_t last_time;
    uint64_t number_of_samples;
    uint64_t difference_sum;
    uint64_t num_bins;
    uint64_t bin_max;
    uint64_t bin_width;
    uint64_t* bins;
    uint64_t max_sample;
    bool doBin;
} Timer;

Timer* timing_createSimpleTimer();

Timer* timing_createTimer(uint64_t num_bins, uint64_t bin_max);

void timing_markTime(Timer*);

double timing_avgIntervalNs(Timer*);

void timing_printHistogram(Timer*);

#endif


#ifdef __cplusplus
}
#endif

