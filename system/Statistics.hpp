
#pragma once

#include <inttypes.h>
#include <glog/logging.h>
#include <sstream>
#include <iostream>

#include "Grappa.hpp"

#ifdef VTRACE_SAMPLED
#include <vt_user.h>
#endif

namespace Grappa {
  
  class StatisticBase {
  protected:
    const char * name;
  public:
    /// registers stat with global stats vector by default (reg_new=false to disable)
    StatisticBase(const char * name, bool reg_new = true);
    
    /// prints as a JSON entry
    virtual std::ostream& json(std::ostream&) const = 0;
    
    /// periodic sample (VTrace sampling triggered by GPerf stuff)
    virtual void sample() const = 0;
    
    virtual void merge(StatisticBase*& other_ptr);
  };
  
  using StatisticList = std::vector<StatisticBase const*>;
  
  template<typename T>
  class Statistic : public StatisticBase {
  public:
    T value;

#ifdef VTRACE_SAMPLED
    unsigned vt_counter;
    static const int vt_type;
#endif
    
    Statistic(const char * name, T initial_value, bool reg_new = true):
        value(initial_value), StatisticBase(name, reg_new) {
#ifdef VTRACE_SAMPLED
        if (Statistic::vt_type == -1) {
          LOG(ERROR) << "warning: VTrace sampling unsupported for this type of Statistic.";
        } else {
          vt_counter = VT_COUNT_DEF(name, name, Statistic::vt_type, VT_COUNT_DEFGROUP);
        }
#endif
    }
    
    virtual std::ostream& json(std::ostream& o) const {
      o << '"' << name << "\": \"" << value << '"';
      return o;
    }
    
    virtual void sample() const {
#ifdef VTRACE_SAMPLED
      // vt_sample() specialized for supported tracing types in Statistics.cpp
      vt_sample();
#endif
    }
    
    virtual void merge(StatisticBase*& other_ptr) {
      if (other_ptr == NULL) {
        // first one needs to allocate space, then initializes with its own value
        // (note: must do `reg_new`=false so we don't re-register this stat)
        other_ptr = new Statistic(name, value, false);
      } else {
        // everyone else merges themselves in
        // for this simple statistic, it's as simple as accumulating
        Statistic* o = reinterpret_cast<Statistic*>(other_ptr);
        o->value += value;
      }
    }
    
    inline const Statistic<T>& count() { return (*this)++; }
    
    // <sugar>
    template<typename U>
    inline const Statistic<T>& operator+=(U increment) {
      value += increment;
      return *this;
    }
    template<typename U>
    inline const Statistic<T>& operator-=(U decrement) {
      value -= decrement;
      return *this;
    }
    inline const Statistic<T>& operator++() { return *this += 1; }
    inline const Statistic<T>& operator--() { return *this += -1; }
    inline T operator++(int) { *this += 1; return value; }
    inline T operator--(int) { *this += -1; return value; }
    
    // allow casting as just the value
    inline operator T() const { return value; }
    
    inline Statistic& operator=(T value) {
      this->value = value;
      return *this;
    }
    // </sugar>
  };


  
  namespace Statistics {
    // singleton list
    StatisticList& registered_stats();
    
    void print(std::ostream& out = std::cerr, StatisticList& stats = registered_stats());
    void merge(std::vector<StatisticBase*>& result);
  }
  
} // namespace Grappa

/// make statistics printable
inline std::ostream& operator<<(std::ostream& o, const Grappa::StatisticBase& stat) {
  return stat.json(o);
}

/// Define a new Grappa Statistic
/// @param type: supported types include: int, unsigned int, int64_t, uint64_t, float, and double
#define GRAPPA_DEFINE_STAT(name, type, initial_value) \
  static Grappa::Statistic<type> name(#name, initial_value)

/// Declare a stat (defined in a separate .cpp file) so it can be used
#define GRAPPA_DECLARE_STAT(name, type) \
  extern Grappa::Statistic<type> name;
