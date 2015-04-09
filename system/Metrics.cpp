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

#include "MetricBase.hpp"
#include "Metrics.hpp"
#include "SimpleMetricImpl.hpp"
#include "Grappa.hpp"
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdint>
#include "Collective.hpp"
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;


DEFINE_int64( stats_blob_ticks, 300000000000L, "number of ticks to wait before dumping stats blob");
DEFINE_string( stats_blob_filename, "stats.json", "Stats blob filename" );
DEFINE_bool( stats_blob_enable, true, "Enable stats dumping" );


DECLARE_string(stats_blob_filename);

#ifdef VTRACE_SAMPLED
#include <vt_user.h>
#endif

namespace Grappa {

  impl::MetricBase::MetricBase(const char * name, bool reg_new): name(name) {
    if (reg_new) {
      Grappa::impl::registered_stats().push_back(this);
      // commented out because this gets called before GLOG is intialized
      //VLOG(1) << "registered <" << this->name << ">";
    }
  }
  
  namespace impl {
    MetricList& registered_stats() {
      static MetricList r;
      return r;
    }
    
    char * exe_name;
    int time_for_profiler = 0;
    
    /// Record binary name for use in construcing profiler filenames
    void set_exe_name( char * argv0 ) {
      exe_name = argv0;
      time_for_profiler = time(NULL);
    }
    
    void stat_list_json(std::ostream& out, MetricList& stats) {
      std::ostringstream o;
      for (Grappa::impl::MetricBase*& s : stats) {
        // skip printing "," before first one
        if (&s-&stats[0] != 0) { o << ",\n"; }
        
        s->json(o << "  ");
      }
      out << o.str();
    }
    
    // callback for sampling at profiler signals
    // runs in signal handler, so should be async-signal-safe
    int profile_handler( void * arg ) {
      take_tracing_sample = true;
    #ifdef GOOGLE_PROFILER
      return 1; // tell profiler to profile at this tick
    #endif
    #ifdef STATS_BLOB
      return 0; // don't take profiling sample
    #endif
      return 0;
    }
    
    /// set when we should take a sample at the next context switch
    bool take_tracing_sample = false;

    #define PROFILER_FILENAME_LENGTH 2048
    static char profiler_filename[PROFILER_FILENAME_LENGTH] = {0}; 

    // which profiling phase are we in? 
    static int profiler_phase = 0;
    
    /// Get next filename for profiler files
    char * get_next_profiler_filename( ) {
      // use Slurm environment variables if we can
      // char * jobname = getenv("SLURM_JOB_NAME");
      char * jobid = getenv("SLURM_JOB_ID");
      char * procid = getenv("SLURM_PROCID");
      if( /*jobname != NULL &&*/ jobid != NULL && procid != NULL ) {
        sprintf( profiler_filename, "%s.%s.rank%s.phase%d.prof", "exe", jobid, procid, profiler_phase );
      } else {
        sprintf( profiler_filename, "%s.%d.pid%d.phase%d.prof", exe_name, time_for_profiler, getpid(), profiler_phase );
      }
      VLOG(3) << "profiler_filename = " << profiler_filename;
      ++profiler_phase;
      return profiler_filename;
    }
    
  }
  
  
  
  namespace Metrics {

    void merge(MetricList& result) {
      result.clear(); // ensure it's empty
      
      for (Grappa::impl::MetricBase * local_stat : Grappa::impl::registered_stats()) {
        Grappa::impl::MetricBase* merge_target = local_stat->clone();
        result.push_back(merge_target); // slot for merged stat
        merge_target->merge_all(local_stat);
      }
    }

    void print(std::ostream& out, MetricList& stats, const std::string& legacy_stats) {
      std::ostringstream o;
      o << "STATS{\n";

      impl::stat_list_json(o, stats);
      o << ",\n";

      o << legacy_stats;

      o << "\n}STATS";
      out << o.str() << std::endl;
    }

    void merge_and_print(std::ostream& out) {
      MetricList all;
      merge(all); // also flushes histogram logs

      print(out, all, "");
    }

    void merge_and_dump_to_file() {
      MetricList all;
      merge(all); // also flushes histogram logs

      std::ofstream of( FLAGS_stats_blob_filename.c_str(), std::ios::out );
      print(of, all, "");
    }
    
    void dump_stats_blob() {
      if( FLAGS_stats_blob_enable ) {
        MetricList all;
        merge(all); // also flushes histogram logs

        std::ofstream o( FLAGS_stats_blob_filename.c_str(), std::ios::out );
        print( o, all, "");
      }
    }

    void reset() {
      for (auto* stat : Grappa::impl::registered_stats()) {
        stat->reset();
      }
    }
    
    void reset_all_cores() {
#ifdef HISTOGRAM_SAMPLED
      try {
        char * jobid = getenv("SLURM_JOB_ID");
        char dir[256]; sprintf(dir, "histogram.%s", jobid);
        fs::create_directories(dir);
        fs::permissions(dir, fs::perms::all_all);
      } catch (fs::filesystem_error& e) {
        LOG(ERROR) << "filesystem error: " << e.what();
      }
#endif
      
      call_on_all_cores([]{
        reset();
      });
    }
        
    void start_tracing() {
      call_on_all_cores([]{
        start_tracing_here();
      });
    }

    void start_tracing_here() {
      #ifdef GOOGLE_PROFILER
        ProfilerOptions po;
        po.filter_in_thread = &impl::profile_handler;
        po.filter_in_thread_arg = NULL;
        ProfilerStartWithOptions( impl::get_next_profiler_filename(), &po );
        //ProfilerStart( Grappa_get_profiler_filename() );
      #if defined(VTRACE_SAMPLED) || defined(HISTOGRAM_SAMPLED)
        reset();
      #endif
      #ifdef VTRACE_SAMPLED
        VT_USER_START("sampling");
        sample();
        if (mycore() == 0) LOG(INFO) << "Tracing started.";
      #endif
      #endif // GOOGLE_PROFILER
    }

    void stop_tracing_here() {
      #ifdef GOOGLE_PROFILER
        ProfilerStop( );
        impl::profile_handler(NULL);
        #ifdef VTRACE_SAMPLED
          VT_USER_END("sampling");
          sample();
          if (mycore() == 0) LOG(INFO) << "Tracing stopped.";
        #endif
      #endif
    }

    void stop_tracing() {
      call_on_all_cores([]{
        stop_tracing_here();
      });
    }
    
    void sample() {
      for (auto* stat : Grappa::impl::registered_stats()) {
        stat->sample();
      }
    }
  }
} // namespace Grappa
