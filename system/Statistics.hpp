
#pragma once

#include <iostream>
#include <vector>

#include "StatisticBase.hpp"
#include "SimpleStatistic.hpp"
#include "SummarizingStatistic.hpp"

namespace Grappa {
  
  using StatisticList = std::vector<impl::StatisticBase*>;
  
  namespace impl {
    StatisticList& registered_stats();
  }
  
  namespace Statistics {
    // singleton list
    
    void print(std::ostream& out = std::cerr, StatisticList& stats = Grappa::impl::registered_stats(), const std::string& legacy_stats = "");
    void merge(StatisticList& result);
    void merge_and_print(std::ostream& out = std::cerr);
    void dump_stats_blob();

    void sample_all();
  }
  
} // namespace Grappa

/// make statistics printable
inline std::ostream& operator<<(std::ostream& o, const Grappa::impl::StatisticBase& stat) {
  return stat.json(o);
}

/// Define a new Grappa Statistic
/// @param type: supported types include: int, unsigned int, int64_t, uint64_t, float, and double
#define GRAPPA_DEFINE_STAT(type, name, initial_value) \
  Grappa::type name(#name, initial_value)

/// Declare a stat (defined in a separate .cpp file) so it can be used
#define GRAPPA_DECLARE_STAT(type, name) \
  extern Grappa::type name;
