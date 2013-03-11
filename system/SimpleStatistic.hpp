
#pragma once

#include "StatisticBase.hpp"
#include <glog/logging.h>

#ifdef VTRACE
#include <vt_user.h>
#endif

#ifdef GOOGLE_PROFILER
#include <gperftools/profiler.h>
#endif


namespace Grappa {

  template<typename T>
  class SimpleStatistic : public impl::StatisticBase {
  protected:
    T initial_value;
    T value_;
    
#ifdef VTRACE_SAMPLED
    unsigned vt_counter;
    static const int vt_type;
    
    void vt_sample() const;
#endif
    
  public:
    
    SimpleStatistic(const char * name, T initial_value, bool reg_new = true):
        initial_value(initial_value), value_(initial_value), impl::StatisticBase(name, reg_new) {
#ifdef VTRACE_SAMPLED
        if (SimpleStatistic::vt_type == -1) {
          LOG(ERROR) << "warning: VTrace sampling unsupported for this type of SimpleStatistic.";
        } else {
          vt_counter = VT_COUNT_DEF(name, name, SimpleStatistic::vt_type, VT_COUNT_DEFGROUP);
        }
#endif
    }
    
    virtual std::ostream& json(std::ostream& o) const {
      o << '"' << name << "\": \"" << value_ << '"';
      return o;
    }
    
    virtual void reset() {
      value_ = initial_value;
    }
    
    virtual void sample() const {
#ifdef VTRACE_SAMPLED
      // vt_sample() specialized for supported tracing types in Statistics.cpp
      vt_sample();
#endif
    }
    
    virtual SimpleStatistic<T>* clone() const {
      // (note: must do `reg_new`=false so we don't re-register this stat)
      return new SimpleStatistic<T>(name, value_, false);
    }
    
    virtual void merge_all(impl::StatisticBase* static_stat_ptr);

    inline const SimpleStatistic<T>& count() { return (*this)++; }

    /// Get the current value
    inline T value() const { return value_; }
    
    // <sugar>
    template<typename U>
    inline const SimpleStatistic<T>& operator+=(U increment) {
      value_ += increment;
      return *this;
    }
    template<typename U>
    inline const SimpleStatistic<T>& operator-=(U decrement) {
      value_ -= decrement;
      return *this;
    }
    inline const SimpleStatistic<T>& operator++() { return *this += 1; }
    inline const SimpleStatistic<T>& operator--() { return *this += -1; }
    inline T operator++(int) { *this += 1; return value_; }
    inline T operator--(int) { *this += -1; return value_; }
    
    // allow casting as just the value
    inline operator T() const { return value_; }
    
    inline SimpleStatistic& operator=(T value) {
      this->value_ = value;
      return *this;
    }
    // </sugar>
  };
  
} // namespace Grappa
