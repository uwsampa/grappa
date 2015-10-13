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
