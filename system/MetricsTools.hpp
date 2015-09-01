////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////
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

/// associative reduce function for Metrics
/// StatType must have void merge(const StatType *)
template <typename StatType>
StatType stat_reduce(const StatType& a, const StatType& b) {
  StatType newst = a;
  newst.merge(&b);
  return newst;
}

/// Stats that are totals.
/// A total can be summed, have a max, and a std deviation
class TotalMetric {
  private:
    uint64_t total_;
    uint64_t max_;
    RunningStandardDeviation stddev_;

  public:
    TotalMetric() {
      reset();
    }

    void merge( const TotalMetric& other ) {
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
  (d).add(STRINGIFY(name##_max), name.getMax() ); \
(d).add(STRINGIFY(name##_stddev), name.getStddev() );

#endif // STATISTICS_TOOLS_HPP_

