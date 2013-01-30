
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
    void init();
  public:
    StatisticBase(const char * name);
    
    virtual std::ostream& json(std::ostream&) const = 0;
    virtual void sample() const = 0;
  };
  
  template<typename T>
  class Statistic : public StatisticBase {
  public:
    T value;

#ifdef VTRACE_SAMPLED
    unsigned vt_counter;
    static const int vt_type;
#endif
    
    Statistic(const char * name, T initial_value):
        value(initial_value), StatisticBase(name) {
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
    
    virtual void sample() const;
    
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

#ifdef VTRACE_SAMPLED
  // sample() specialized for supported tracing types in Statistics.cpp
#else
  template <typename T> void Statistic<T>::sample() const {}
#endif

  void PrintStatistics(std::ostream& out = std::cerr);
  
} // namespace Grappa

inline std::ostream& operator<<(std::ostream& o, const Grappa::StatisticBase& stat) {
  return stat.json(o);
}

/// Define a new Grappa Statistic
/// @param type: supported types include: int, unsigned int, int64_t, uint64_t, float, and double
#define GRAPPA_DEFINE_STAT(name, type, initial_value) \
  static Grappa::Statistic<type> name(#name, initial_value)

#define GRAPPA_DEFINE_STAT(name, type) \
  extern Grappa::Statistic<type> name;
