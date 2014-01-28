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

#include "MetricBase.hpp"
#include <glog/logging.h>

#ifdef VTRACE
#include <vt_user.h>
#endif

#ifdef GOOGLE_PROFILER
#include <gperftools/profiler.h>
#endif

#include <functional>


namespace Grappa {
  /// @addtogroup Utility
  /// @{

  /// Metric that simply keeps track of a single value over time.
  /// Typically used as a counter, but can also be used for sampling an instantaneous value.
  template<typename T>
  class SimpleMetric : public impl::MetricBase {
  protected:
    typedef std::function<T(void)> InitFn;
    T initial_value;
    T value_;
    InitFn initf_;
    
#ifdef VTRACE_SAMPLED
    unsigned vt_counter;
    static const int vt_type;
    
    void vt_sample() const;
#endif
    
  public:
    
    SimpleMetric(const char * name, T initial_value, bool reg_new = true):
        initial_value(initial_value), value_(initial_value), initf_(NULL), impl::MetricBase(name, reg_new) {
#ifdef VTRACE_SAMPLED
        if (SimpleMetric::vt_type == -1) {
          LOG(ERROR) << "warning: VTrace sampling unsupported for this type of SimpleMetric.";
        } else {
          vt_counter = VT_COUNT_DEF(name, name, SimpleMetric::vt_type, VT_COUNT_DEFGROUP);
        }
#endif
    }
   
   // reset using function 
   // IMPORTANT: currently, this forces the merge to do a min instead of a sum
    SimpleMetric(const char * name, InitFn initf, bool reg_new = true):
        value_(initf()), initf_(initf), impl::MetricBase(name, reg_new) {
#ifdef VTRACE_SAMPLED
        if (SimpleMetric::vt_type == -1) {
          LOG(ERROR) << "warning: VTrace sampling unsupported for this type of SimpleMetric.";
        } else {
          vt_counter = VT_COUNT_DEF(name, name, SimpleMetric::vt_type, VT_COUNT_DEFGROUP);
        }
#endif
    }
    
    virtual std::ostream& json(std::ostream& o) const {
      o << '"' << name << "\": " << value_;
      return o;
    }
    
    virtual void reset() {
      if (initf_ != NULL) {
        value_ = initf_();
      } else {
        value_ = initial_value;
      }
    }
    
    virtual void sample() {
#ifdef VTRACE_SAMPLED
      // vt_sample() specialized for supported tracing types in Metrics.cpp
      vt_sample();
#endif
    }
    
    virtual SimpleMetric<T>* clone() const {
      // (note: must do `reg_new`=false so we don't re-register this stat)
      return new SimpleMetric<T>(name, value_, false);
    }
    
    virtual void merge_all(impl::MetricBase* static_stat_ptr);

    inline const SimpleMetric<T>& count() { return (*this)++; }

    /// Get the current value
    inline T value() const { return value_; }
    
    // <sugar>
    template<typename U>
    inline const SimpleMetric<T>& operator+=(U increment) {
      value_ += increment;
      return *this;
    }
    template<typename U>
    inline const SimpleMetric<T>& operator-=(U decrement) {
      value_ -= decrement;
      return *this;
    }
    inline const SimpleMetric<T>& operator++() { return *this += 1; }
    inline const SimpleMetric<T>& operator--() { return *this += -1; }
    inline T operator++(int) { *this += 1; return value_; }
    inline T operator--(int) { *this += -1; return value_; }
    
    // allow casting as just the value
    inline operator T() const { return value_; }
    
    inline SimpleMetric& operator=(T value) {
      this->value_ = value;
      return *this;
    }
    // </sugar>
  };
  
  /// @}
} // namespace Grappa
