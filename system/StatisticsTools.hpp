#ifndef _STATISTICS_TOOLS_HPP_
#define _STATISTICS_TOOLS_HPP_

#include <cmath>
#include <glog/logging.h>

inline double inc_avg(double curr_avg, uint64_t count, double val) {
	return curr_avg + (val-curr_avg)/(count);
}

inline uint64_t max2(uint64_t a, uint64_t b) {
  return (a>b) ? a : b;
}



class RunningStandardDeviation {
  private:
    double s0;
    double s1;
    double s2;

  public:
    RunningStandardDeviation() {
      reset();
    }

    void reset() {
        s0 = 0.0;
        s1 = 0.0;
        s2 = 0.0;
    }


    void addSample( uint64_t value ) {
        s0 += 1;
        s1 += value;
        s2 += pow(value, 2);
    }
    
    double value() {
      double sq = s0*s2 - pow(s1, 2);
      if (sq > 0 && s0 > 0) {
        return sqrt(sq) / s0;
      } else {
        return 0;
      }
    }

    void merge( RunningStandardDeviation other ) {
      s0 += other.s0;
      s1 += other.s1;
      s2 += other.s2;
    }
};

/// associative reduce function for Statistics
/// StatType must have void merge(const StatType *)
template <typename StatType>
StatType stat_reduce(const StatType& a, const StatType& b) {
  StatType newst = a;
  newst.merge(&b);
  return newst;
}

#endif // STATISTICS_TOOLS_HPP_
