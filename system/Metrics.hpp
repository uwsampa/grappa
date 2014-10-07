////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
////////////////////////////////////////////////////////////////////////

#pragma once

#include <iostream>
#include <vector>

#include "MetricBase.hpp"
#include "SimpleMetric.hpp"
#include "StringMetric.hpp"
#include "SummarizingMetric.hpp"
#include "CallbackMetric.hpp"
#include "MaxMetric.hpp"

/// @addtogroup Utility
/// @{

namespace Grappa {
  
  using MetricList = std::vector<impl::MetricBase*>;
  
  namespace impl {
    /// singleton list of stats
    MetricList& registered_stats();
    
    extern bool take_tracing_sample;
    void set_exe_name( char * name );
  }
  
  namespace Metrics {
        
    
    /// Print all registered stats in JSON format. Takes another argument for including
    /// legacy stats in output (inside "STATS{  }STATS" bookends)
    void print(std::ostream& out = std::cerr, MetricList& stats = Grappa::impl::registered_stats(), const std::string& legacy_stats = "");
    
    /// Merge registered stats from all cores into `result` list.
    /// @param result   must be a clone of a core's impl::registered_stats().
    void merge(MetricList& result);
    
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

    /// Only call 'start_tracing' on this core (use in SPMD context)
    void start_tracing_here();
    
    /// Stop recording tracing and profiling information. Trace/profile is written out and aggregated 
    /// at end of execution.
    void stop_tracing();
    
    /// Only call 'stop_tracing' on this core (use in SPMD context)
    void stop_tracing_here();
  }
  
} // namespace Grappa

/// make statistics printable
inline std::ostream& operator<<(std::ostream& o, const Grappa::impl::MetricBase& stat) {
  return stat.json(o);
}

/// Define a new Grappa Metric
/// @param type  Metric type (e.g. SummarizingMetric<double>)
/// @param name  Variable name
/// @param arg1  Default value
#define GRAPPA_DEFINE_METRIC(type, name, arg1) \
  Grappa::type name(#name, arg1)

/// Declare a metric (defined in a separate .cpp file) so it can be used
/// 
/// @param type  Metric type (e.g. SummarizingMetric<double>)
/// @param name  Variable name
#define GRAPPA_DECLARE_METRIC(type, name) \
  extern Grappa::type name;


/// @}
