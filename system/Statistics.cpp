
#include "Statistics.hpp"
#include <vector>
#include <iostream>
#include <sstream>
#include <cstdint>

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
  
  
  StatisticBase::StatisticBase(const char * name): name(name) {
    Statistics::registered_stats().push_back(this);
    VLOG(1) << "registered <" << this->name << ">";
  }
  
  namespace Statistics {
    
    StatisticList& registered_stats() {
      static StatisticList r;
      return r;
    }
    
    void merge(StatisticList& result) {
      if (result != registered_stats()) {
        result.insert(result.end(), registered_stats().begin(), registered_stats().end());
      }
      // TODO: issue a bunch of delegate requests in parallel
    }
    
    void print(std::ostream& out, StatisticList& stats) {
      std::ostringstream o;
      o << "STATS{\n";
      for (const StatisticBase*& s : stats) {
        // skip printing "," before first one
        if (&s-&stats[0] != 0) { o << ",\n"; }
        
        o << "  " << *s;
      }
      o << "\n}STATS";
      out << o.str() << std::endl;
    }
  }
} // namespace Grappa