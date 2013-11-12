#include "StatisticBase.hpp"
#include "Statistics.hpp"
#include "SimpleStatisticImpl.hpp"
#include "Grappa.hpp"
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdint>
#include "Collective.hpp"
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;



DECLARE_string(stats_blob_filename);

#ifdef VTRACE_SAMPLED
#include <vt_user.h>
#endif

namespace Grappa {

  impl::StatisticBase::StatisticBase(const char * name, bool reg_new): name(name) {
    if (reg_new) {
      Grappa::impl::registered_stats().push_back(this);
      // commented out because this gets called before GLOG is intialized
      //VLOG(1) << "registered <" << this->name << ">";
    }
  }
  
  namespace impl {
    StatisticList& registered_stats() {
      static StatisticList r;
      return r;
    }
  }
  
  namespace Statistics {
    namespace impl {
      void stat_list_json(std::ostream& out, StatisticList& stats) {
        std::ostringstream o;
        for (Grappa::impl::StatisticBase*& s : stats) {
          // skip printing "," before first one
          if (&s-&stats[0] != 0) { o << ",\n"; }
          
          s->json(o << "  ");
        }
        out << o.str();
      }
    }

    void merge(StatisticList& result) {
      result.clear(); // ensure it's empty
      
      for (Grappa::impl::StatisticBase * local_stat : Grappa::impl::registered_stats()) {
        Grappa::impl::StatisticBase* merge_target = local_stat->clone();
        result.push_back(merge_target); // slot for merged stat
        merge_target->merge_all(local_stat);
      }
    }

    void print(std::ostream& out, StatisticList& stats, const std::string& legacy_stats) {
      std::ostringstream o;
      o << "STATS{\n";

      impl::stat_list_json(o, stats);
      o << ",\n";

      o << legacy_stats;

      o << "\n}STATS";
      out << o.str() << std::endl;
    }

    void merge_and_print(std::ostream& out) {
      StatisticList all;
      merge(all); // also flushes histogram logs

      print(out, all, "");
    }

    void merge_and_dump_to_file() {
      StatisticList all;
      merge(all); // also flushes histogram logs

      std::ofstream of( FLAGS_stats_blob_filename.c_str(), std::ios::out );
      print(of, all, "");
    }
    
    void dump_stats_blob() {
      std::ofstream o( FLAGS_stats_blob_filename.c_str(), std::ios::out );
      print( o, Grappa::impl::registered_stats(), "");
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
        Grappa_start_profiling();
        reset();
      });
    }

    void stop_tracing() {
      call_on_all_cores([]{
        Grappa_stop_profiling();
      });
    }
    
    void sample() {
      for (auto* stat : Grappa::impl::registered_stats()) {
        stat->sample();
      }
    }
  }
} // namespace Grappa
