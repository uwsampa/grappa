
#pragma once

#include <cmath>

#include "StatisticBase.hpp"

namespace Grappa {

  template<typename T>
  class SummarizingStatistic : public impl::StatisticBase {
  protected:

    T value;
    size_t n;
    double mean;
    double M2;

    void process( T t ) {
      n++;
      double delta = t - mean;
      mean += delta / n;
      M2 += delta * (t - mean);
    }
    
    double variance() const {
      return M2 / (n - 1);
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
    
    inline void vt_sample() const;
#endif
    
  public:
    
    SummarizingStatistic(const char * name, T initial_value, bool reg_new = true)
      : impl::StatisticBase(name, reg_new) 
      , value(initial_value)
      , n(0) // TODO: this assumes initial_value is not actually a value
      , mean(initial_value)
      , M2(0) {
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
      , value( s.value )
      , n( s.n )
      , mean( s.mean )
      , M2( s.M2 ) {
      // no vampir registration since this is for merging
    }
    
    virtual std::ostream& json(std::ostream& o) const {
      o << '"' << name << "\": \"" << value << "\", ";
      o << '"' << name << "_count\": \"" << n << "\", ";
      o << '"' << name << "_mean\": \"" << mean << "\", ";
      o << '"' << name << "_stddev\": \"" << stddev() << "\"";
      return o;
    }
    
    virtual void sample() const {
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
    
    // <sugar>
    template<typename U>
    inline const SummarizingStatistic<T>& operator+=(U increment) {
      value += increment;
      process( increment );
      return *this;
    }
    template<typename U>
    inline const SummarizingStatistic<T>& operator-=(U decrement) {
      value -= decrement;
      process( decrement );
      return *this;
    }
    inline const SummarizingStatistic<T>& operator++() { return *this += 1; }
    inline const SummarizingStatistic<T>& operator--() { return *this += -1; }
    inline T operator++(int) { *this += 1; return value; }
    inline T operator--(int) { *this += -1; return value; }
    
    // allow casting as just the value
    inline operator T() const { return value; }
    
    inline SummarizingStatistic& operator=( const SummarizingStatistic<T>& t ) {
      this->value = t.value;
      this->n = t.n;
      this->mean = t.mean;
      this->M2 = t.M2;
      return *this;
    }

    inline SummarizingStatistic& operator=(T value) {
      this->value = value;
      process( value );
      return *this;
    }
    // </sugar>
  };
  
} // namespace Grappa
