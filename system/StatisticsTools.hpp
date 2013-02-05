// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef STATISTICS_TOOLS_HPP
#define STATISTICS_TOOLS_HPP

#include <cmath>
#include <glog/logging.h>


/// Utilities for keeping and calculating runtime statistics

/// Incremental average
/// @param curr_avg the current running average
/// @param count the new total count of averaged values
/// @param val the new value to include in the average
/// @return the new running average
inline double inc_avg(double curr_avg, uint64_t count, double val) {
  return curr_avg + (val-curr_avg)/(count);
}

/// Max of two unsigned integers
inline uint64_t max2(uint64_t a, uint64_t b) {
  return (a>b) ? a : b;
}


/// Calculates a running standard deviation
class RunningStandardDeviation {
  private:
    double s0;
    double s1;
    double s2;

  public:
    /// Construct new running standard deviation with 0 samples
    RunningStandardDeviation() {
      reset();
    }

    /// reset to 0 samples
    void reset() {
      s0 = 0.0;
      s1 = 0.0;
      s2 = 0.0;
    }

    /// Add a new sample
    void addSample( uint64_t value ) {
      s0 += 1;
      s1 += value;
      s2 += pow(value, 2);
    }

    /// Get the current standard deviation of all added samples 
    double value() {
      double sq = s0*s2 - pow(s1, 2);
      if (sq > 0 && s0 > 0) {
        return sqrt(sq) / s0;
      } else {
        return 0;
      }
    }

    /// merge with another group of samples
    /// this is a mutating method
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

/// Stats that are totals.
/// A total can be summed, have a max, and a std deviation
class TotalStatistic {
  private:
    uint64_t total_;
    uint64_t max_;
    RunningStandardDeviation stddev_;

  public:
    TotalStatistic() {
      reset();
    }

    void merge( const TotalStatistic& other ) {
      total_ += other.total_;
      max_ = max2( max_, other.max_ );
      stddev_.merge( other.stddev_ );
    }

    void reset() {
      total_ = 0;
      max_ = 0;
      stddev_.reset();
    }

    void update( uint64_t val ) {
      total_ += val;
      max_ = max2( max_, val );
      stddev_.addSample( val );
    }

    uint64_t getTotal() {
      return total_;
    }

    uint64_t getMax() {
      return max_;
    }

    double getStddev() {
      return stddev_.value();
    }
};

// convenience macros for merging and printing stats
#define MERGE_STAT_COUNT( name, other ) name += (other)->name
#define MERGE_STAT_TOTAL( name, other ) name.merge((other)->name)

#define STRINGIFY(s) #s
#define DICT_ADD_STAT_TOTAL( d, name ) (d).add(#name, name.getTotal() ); \
  (d).add(STRINGIFY(max_##name), name.getMax() ); \
(d).add(STRINGIFY(stddev_##name), name.getStddev() );

#endif // STATISTICS_TOOLS_HPP_

