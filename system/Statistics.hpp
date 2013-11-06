
#pragma once

#include <iostream>
#include <vector>

#include "StatisticBase.hpp"
#include "SimpleStatistic.hpp"
#include "SummarizingStatistic.hpp"
#include "MaxStatistic.hpp"

/// @addtogroup Utility
/// @{

namespace Grappa {
  
  using StatisticList = std::vector<impl::StatisticBase*>;
  
  namespace impl {
    StatisticList& registered_stats();
  }
  
  namespace Statistics {
    // singleton list
    
    /// Print all registered stats in JSON format. Takes another argument for including
    /// legacy stats in output (inside "STATS{  }STATS" bookends)
    void print(std::ostream& out = std::cerr, StatisticList& stats = Grappa::impl::registered_stats(), const std::string& legacy_stats = "");
    
    /// Merge registered stats from all cores into `result` list.
    /// @param result   must be a clone of a core's impl::registered_stats().
    void merge(StatisticList& result);
    
    /// Create clone of stats list, merge all stats into it, and print them.
    void merge_and_print(std::ostream& out = std::cerr);
    
    /// Merge stats from all cores and dump them to file specified by '--stats_blob_filename'
    void merge_and_dump_to_file();
    
    /// Dump local registered stats to file.
    void dump_stats_blob();
    
    /// Sample all local registered stats
    void sample();
    
    /// Reset all local registered stats
    void reset();
    
    /// Do `reset()` on all cores (uses `Grappa::call_on_all_cores`)
    void reset_all_cores();
    
    /// Begin recording stats using VampirTrace (also enables Google gperf profiling) (also resets stats)
    void start_tracing();
    
    /// Stop recording tracing and profiling information. Trace/profile is written out and aggregated 
    /// at end of execution.
    void stop_tracing();
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

/// @}
