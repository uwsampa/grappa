
#pragma once

#include <cmath>

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
  class SummarizingStatistic : public impl::StatisticBase {
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
    
    SummarizingStatistic(const char * name, T initial_value, bool reg_new = true)
      : impl::StatisticBase(name, reg_new)
      , initial_value(initial_value)
      , value_(initial_value)
      , n(0) // TODO: this assumes initial_value is not actually a value
      , mean(initial_value)
      , M2(0)
      , min(0)
      , max(0) {
#ifdef VTRACE_SAMPLED
      if (SummarizingStatistic::vt_type == -1) {
        LOG(ERROR) << "warning: VTrace sampling unsupported for this type of SummarizingStatistic.";
      } else {
        vt_counter_value = VT_COUNT_DEF(name, name, SummarizingStatistic::vt_type, VT_COUNT_DEFGROUP);
        // TODO: add traces for summary statistics
        //vt_counter_count = VT_COUNT_DEF(name, name, , VT_COUNT_DEFGROUP);
      }
#endif
    }

    SummarizingStatistic(const SummarizingStatistic& s ) 
      : impl::StatisticBase( s.name, false )
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
      // vt_sample() specialized for supported tracing types in Statistics.cpp
      vt_sample();
#endif
    }
    
    virtual SummarizingStatistic<T>* clone() const {
      return new SummarizingStatistic<T>( *this );
    }
    
    virtual void merge_all(impl::StatisticBase* static_stat_ptr);

    inline const SummarizingStatistic<T>& count() { return (*this)++; }

    /// Get the current value
    inline T value() const { return value_; }
    
    // <sugar>
    template<typename U>
    inline const SummarizingStatistic<T>& operator+=(U increment) {
      value_ += increment;
      process( increment );
      return *this;
    }
    template<typename U>
    inline const SummarizingStatistic<T>& operator-=(U decrement) {
      value_ -= decrement;
      process( decrement );
      return *this;
    }
    inline const SummarizingStatistic<T>& operator++() { return *this += 1; }
    inline const SummarizingStatistic<T>& operator--() { return *this += -1; }
    inline T operator++(int) { *this += 1; return value_; }
    inline T operator--(int) { *this += -1; return value_; }
    
    // allow casting as just the value
    inline operator T() const { return value_; }
    
    inline SummarizingStatistic& operator=( const SummarizingStatistic<T>& t ) {
      this->value_ = t.value_;
      this->n = t.n;
      this->mean = t.mean;
      this->M2 = t.M2;
      return *this;
    }

    inline SummarizingStatistic& operator=(T value) {
      this->value_ = value;
      process( value );
      return *this;
    }
    // </sugar>
  };
  
} // namespace Grappa
