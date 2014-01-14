
#pragma once

#include "MetricBase.hpp"
#include <glog/logging.h>

#include <algorithm> // max

#ifdef VTRACE
#include <vt_user.h>
#endif

#ifdef GOOGLE_PROFILER
#include <gperftools/profiler.h>
#endif


namespace Grappa {
  /// @addtogroup Utility
  /// @{

  /// Metric that simply keeps track of a single value over time.
  /// Typically used as a counter, but can also be used for sampling an instantaneous value.
  template<typename T>
  class MaxMetric : public impl::MetricBase {
  protected:
    T initial_value;
    T value_;
    
#ifdef VTRACE_SAMPLED
    unsigned vt_counter;
    static const int vt_type;
    
    void vt_sample() const;
#endif
    
  public:
    
    MaxMetric(const char * name, T initial_value, bool reg_new = true):
        initial_value(initial_value), value_(initial_value), impl::MetricBase(name, reg_new) {
#ifdef VTRACE_SAMPLED
        if (MaxMetric::vt_type == -1) {
          LOG(ERROR) << "warning: VTrace sampling unsupported for this type of MaxMetric.";
        } else {
          vt_counter = VT_COUNT_DEF(name, name, MaxMetric::vt_type, VT_COUNT_DEFGROUP);
        }
#endif
    }
    
    virtual std::ostream& json(std::ostream& o) const {
      o << '"' << name << "\": " << value_;
      return o;
    }
    
    virtual void reset() {
      value_ = initial_value;
    }
    
    virtual void sample() {
#ifdef VTRACE_SAMPLED
      // vt_sample() specialized for supported tracing types in Metrics.cpp
      vt_sample();
#endif
    }
    
    virtual MaxMetric<T>* clone() const {
      // (note: must do `reg_new`=false so we don't re-register this stat)
      return new MaxMetric<T>(name, value_, false);
    }
    
    virtual void merge_all(impl::MetricBase* static_stat_ptr);

    inline const MaxMetric<T>& count() { return (*this)++; }

    /// Get the current value
    inline T value() const { return value_; }
    
    // allow casting as just the value
    inline operator T() const { return value_; }
    
    inline void add(T value) {
      this->value_ = std::max(this->value_,value);
    }
  };
  
  /// @}
} // namespace Grappa
