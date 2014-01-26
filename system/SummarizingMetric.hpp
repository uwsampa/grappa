////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
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
