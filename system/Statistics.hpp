
#pragma once

#include <iostream>

#include "StatisticBase.hpp"
#include "SimpleStatistic.hpp"

namespace Grappa {
  
  using StatisticList = std::vector<impl::StatisticBase*>;
  
  namespace impl {
    StatisticList& registered_stats();
  }
  
  namespace Statistics {
    // singleton list
    
    void print(std::ostream& out = std::cerr, StatisticList& stats = Grappa::impl::registered_stats());
    void merge(StatisticList& result);
  }
  
} // namespace Grappa

/// make statistics printable
inline std::ostream& operator<<(std::ostream& o, const Grappa::impl::StatisticBase& stat) {
  return stat.json(o);
}

/// Define a new Grappa Statistic
/// @param type: supported types include: int, unsigned int, int64_t, uint64_t, float, and double
#define GRAPPA_DEFINE_STAT(type, name, initial_value) \
  static Grappa::type name(#name, initial_value)

/// Declare a stat (defined in a separate .cpp file) so it can be used
#define GRAPPA_DECLARE_STAT(type, name) \
  extern Grappa::type name;
