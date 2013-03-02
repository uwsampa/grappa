#include "StatisticBase.hpp"
#include "Statistics.hpp"
#include "SimpleStatisticImpl.hpp"
#include "Grappa.hpp"
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdint>

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
          
          o << "  " << *s;
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
      merge(all);

      std::ostringstream legacy_stats;
      legacy_reduce_stats_and_dump(legacy_stats);

      print(out, all, legacy_stats.str());
    }

    void dump_stats_blob() {
      std::ostringstream legacy_stats;
      legacy_dump_stats(legacy_stats);

      std::ofstream o( FLAGS_stats_blob_filename.c_str(), std::ios::out );
      print( o, Grappa::impl::registered_stats(), legacy_stats.str());
    }

    void reset() {
      for (auto* stat : Grappa::impl::registered_stats()) {
        stat->reset();
      }
      Grappa_reset_stats();
    }
    

    void sample_all() {
      for (auto* stat : Grappa::impl::registered_stats()) {
        stat->sample();
      }
      legacy_profiling_sample();
    }
  }
} // namespace Grappa
