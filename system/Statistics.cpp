
#include "Statistics.hpp"
#include <vector>
#include <iostream>
#include <sstream>
#include <cstdint>

#ifdef VTRACE_SAMPLED
#include <vt_user.h>
#endif

namespace Grappa {

#ifdef VTRACE_SAMPLED
  template <> inline void Statistic<int>::vt_sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter, value);
  }
  template <> inline void Statistic<int64_t>::vt_sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter, value);
  }
  template <> inline void Statistic<unsigned>::vt_sample() const {
    VT_COUNT_UNSIGNED_VAL(vt_counter, value);
  }
  template <> inline void Statistic<uint64_t>::vt_sample() const {
    VT_COUNT_UNSIGNED_VAL(vt_counter, value);
  }
  template <> inline void Statistic<double>::vt_sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter, value);
  }
  template <> inline void Statistic<float>::vt_sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter, value);
  }
  
  template <> const int Statistic<int>::vt_type = VT_COUNT_TYPE_SIGNED;
  template <> const int Statistic<int64_t>::vt_type = VT_COUNT_TYPE_SIGNED;
  template <> const int Statistic<unsigned>::vt_type = VT_COUNT_TYPE_UNSIGNED;
  template <> const int Statistic<uint64_t>::vt_type = VT_COUNT_TYPE_UNSIGNED;
  template <> const int Statistic<double>::vt_type = VT_COUNT_TYPE_DOUBLE;
  template <> const int Statistic<float>::vt_type = VT_COUNT_TYPE_FLOAT;
#endif
  
  StatisticBase::StatisticBase(const char * name, bool reg_new): name(name) {
    if (reg_new) {
      Statistics::registered_stats().push_back(this);
      VLOG(1) << "registered <" << this->name << ">";
    }
  }
  
  namespace Statistics {
    
    StatisticList& registered_stats() {
      static StatisticList r;
      return r;
    }
    
    void merge(StatisticList& result) {
      result.clear(); // ensure it's empty
      
      for (StatisticBase * local_stat : registered_stats()) {
        StatisticBase* merge_target = local_stat->clone();
        result.push_back(merge_target); // slot for merged stat
        merge_target->merge_all(local_stat);
      }
    }
    
    void print(std::ostream& out, StatisticList& stats) {
      std::ostringstream o;
      o << "STATS{\n";
      for (StatisticBase*& s : stats) {
        // skip printing "," before first one
        if (&s-&stats[0] != 0) { o << ",\n"; }
        
        o << "  " << *s;
      }
      o << "\n}STATS";
      out << o.str() << std::endl;
    }
  }
} // namespace Grappa