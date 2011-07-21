
#ifndef TIMING_HPP_
#define TIMING_HPP_


#include <string>


#define NS_PER_TICK 2.66f

class Timer {
    private:
        uint64_t last_time;
        uint64_t number_of_samples;
        uint64_t difference_sum;
        uint64_t num_bins;
        uint64_t bin_max;
        uint64_t bin_width;
        uint64_t* bins;
        uint64_t max_sample;
        bool doBin;
        bool doBinPrint;
        std::string name;

        void printHistogram();
   
   public:
         Timer(std::string name, uint64_t num_bins, uint64_t bin_max, bool binning);
        ~Timer();

       static Timer* createSimpleTimer(std::string name);
        static Timer* createTimer(std::string name, uint64_t num_bins, uint64_t bin_max);
        void markTime();
        double avgIntervalNs();

        void setBinPrint(bool on);
};

#endif


