#include "StatisticBase.hpp"
#include "Statistics.hpp"
#include "Grappa.hpp"
#include <vector>
#include <iostream>
#include <sstream>
#include <cstdint>


#ifdef VTRACE_SAMPLED
#include <vt_user.h>
#endif

namespace Grappa {

#ifdef VTRACE_SAMPLED
  template <> inline void SimpleStatistic<int>::vt_sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter, value);
  }
  template <> inline void SimpleStatistic<int64_t>::vt_sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter, value);
  }
  template <> inline void SimpleStatistic<unsigned>::vt_sample() const {
    VT_COUNT_UNSIGNED_VAL(vt_counter, value);
  }
  template <> inline void SimpleStatistic<uint64_t>::vt_sample() const {
    VT_COUNT_UNSIGNED_VAL(vt_counter, value);
  }
  template <> inline void SimpleStatistic<double>::vt_sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter, value);
  }
  template <> inline void SimpleStatistic<float>::vt_sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter, value);
  }
  
  template <> const int SimpleStatistic<int>::vt_type = VT_COUNT_TYPE_SIGNED;
  template <> const int SimpleStatistic<int64_t>::vt_type = VT_COUNT_TYPE_SIGNED;
  template <> const int SimpleStatistic<unsigned>::vt_type = VT_COUNT_TYPE_UNSIGNED;
  template <> const int SimpleStatistic<uint64_t>::vt_type = VT_COUNT_TYPE_UNSIGNED;
  template <> const int SimpleStatistic<double>::vt_type = VT_COUNT_TYPE_DOUBLE;
  template <> const int SimpleStatistic<float>::vt_type = VT_COUNT_TYPE_FLOAT;
#endif
  
  impl::StatisticBase::StatisticBase(const char * name, bool reg_new): name(name) {
    if (reg_new) {
      Grappa::impl::registered_stats().push_back(this);
      VLOG(1) << "registered <" << this->name << ">";
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

      print(std::cerr, all, legacy_stats.str());
    }
  }
} // namespace Grappa
