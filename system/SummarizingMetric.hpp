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

#pragma once

#include <cmath>

#include "MetricBase.hpp"
#include <glog/logging.h>

#ifdef VTRACE
#include <vt_user.h>
#endif

#ifdef GOOGLE_PROFILER
#include <gperftools/profiler.h>
#endif

#include <math.h>

#include <limits>

namespace Grappa {

  template<typename T>
  class SummarizingMetric : public impl::MetricBase {
  protected:
    const T initial_value;
    T value_;
    size_t n;
    double mean;
    double M2;
    T min;
    T max;

    void process( T t ) {
      n++;
      double delta = t - mean;
      mean += delta / n;
      M2 += delta * (t - mean);
      if (n == 1) {
        min = t;
        max = t;
      }
      if (t > max) max = t;
      if (t < min) min = t;
    }
    
    double variance() const {
      return (n<2) ? 0.0 : M2 / (n - 1);
    }

    double stddev() const {
      return sqrt( variance() );
    }

#ifdef VTRACE_SAMPLED
    unsigned vt_counter_value;
    unsigned vt_counter_count;
    unsigned vt_counter_mean;
    unsigned vt_counter_stddev;
    static const int vt_type;
    
    void vt_sample() const;
#endif
    
  public:
    
    SummarizingMetric(const char * name, T initial_value, bool reg_new = true)
      : impl::MetricBase(name, reg_new)
      , initial_value(initial_value)
      , value_(initial_value)
      , n(0) // TODO: this assumes initial_value is not actually a value
      , mean(initial_value)
      , M2(0)
      , min(std::numeric_limits<T>::max())
      , max(std::numeric_limits<T>::min()) {
#ifdef VTRACE_SAMPLED
      if (SummarizingMetric::vt_type == -1) {
        LOG(ERROR) << "warning: VTrace sampling unsupported for this type of SummarizingMetric.";
      } else {
        std::string namestr( name );
        std::string countstr = namestr + "_count";
        std::string meanstr = namestr + "_mean";
        std::string stddevstr = namestr + "_stddev";
        
        vt_counter_value  = VT_COUNT_DEF(name,              name,              SummarizingMetric::vt_type, VT_COUNT_DEFGROUP);
        vt_counter_count  = VT_COUNT_DEF(countstr.c_str(),  countstr.c_str(),  VT_COUNT_TYPE_UNSIGNED,        VT_COUNT_DEFGROUP);
        vt_counter_mean   = VT_COUNT_DEF(meanstr.c_str(),   meanstr.c_str(),   VT_COUNT_TYPE_DOUBLE,          VT_COUNT_DEFGROUP);
        vt_counter_stddev = VT_COUNT_DEF(stddevstr.c_str(), stddevstr.c_str(), VT_COUNT_TYPE_DOUBLE,          VT_COUNT_DEFGROUP);
      }
#endif
    }

    SummarizingMetric(const SummarizingMetric& s ) 
      : impl::MetricBase( s.name, false )
      , initial_value(s.initial_value)
      , value_( s.value_ )
      , n( s.n )
      , mean( s.mean )
      , M2( s.M2 ) {
      // no vampir registration since this is for merging
    }
    
    virtual void reset() {
      value_ = initial_value;
      n = 0;
      mean = M2 = 0;
      min = std::numeric_limits<T>::max();
      max = std::numeric_limits<T>::min();
    }
    
    virtual std::ostream& json(std::ostream& o) const {
      o << '"' << name << "\": " << value_ << ", ";
      o << '"' << name << "_count\": " << n << ", ";
      o << '"' << name << "_mean\": " << mean << ", ";
      o << '"' << name << "_stddev\": " << stddev() << ", ";
      o << '"' << name << "_min\": " << min << ", ";
      o << '"' << name << "_max\": " << max;
      return o;
    }
    
    virtual void sample() {
#ifdef VTRACE_SAMPLED
      // vt_sample() specialized for supported tracing types in Metrics.cpp
      vt_sample();
#endif
    }
    
    virtual SummarizingMetric<T>* clone() const {
      return new SummarizingMetric<T>( *this );
    }
    
    virtual void merge_all(impl::MetricBase* static_stat_ptr);

    inline const SummarizingMetric<T>& count() { return (*this)++; }

    /// Get the current value
    inline T value() const { return value_; }
    
    // <sugar>
    template<typename U>
    inline const SummarizingMetric<T>& operator+=(U increment) {
      value_ += increment;
      process( increment );
      return *this;
    }
    template<typename U>
    inline const SummarizingMetric<T>& operator-=(U decrement) {
      value_ -= decrement;
      process( decrement );
      return *this;
    }
    inline const SummarizingMetric<T>& operator++() { return *this += 1; }
    inline const SummarizingMetric<T>& operator--() { return *this += -1; }
    inline T operator++(int) { *this += 1; return value_; }
    inline T operator--(int) { *this += -1; return value_; }
    
    // allow casting as just the value
    inline operator T() const { return value_; }
    
    inline SummarizingMetric& operator=( const SummarizingMetric<T>& t ) {
      this->value_ = t.value_;
      this->n = t.n;
      this->mean = t.mean;
      this->M2 = t.M2;
      this->max = t.max;
      this->min = t.min;
      return *this;
    }

    inline SummarizingMetric& operator=(T value) {
      this->value_ = value;
      process( value );
      return *this;
    }
    // </sugar>
  };
  
} // namespace Grappa
