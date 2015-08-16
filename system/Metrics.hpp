////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
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
