
#include "Statistics.hpp"
#include <vector>
#include <iostream>
#include <sstream>

namespace Grappa {

#ifdef VTRACE_SAMPLED
  template <> void Statistic<int>::sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter, value);
  }
  template <> void Statistic<int64_t>::sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter, value);
  }
  template <> void Statistic<unsigned>::sample() const {
    VT_COUNT_UNSIGNED_VAL(vt_counter, value);
  }
  template <> void Statistic<uint64_t>::sample() const {
    VT_COUNT_UNSIGNED_VAL(vt_counter, value);
  }
  template <> void Statistic<double>::sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter, value);
  }
  template <> void Statistic<float>::sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter, value);
  }
  
  template <> const int Statistic<int>::vt_type = VT_COUNT_TYPE_SIGNED;
  template <> const int Statistic<int64_t>::vt_type = VT_COUNT_TYPE_SIGNED;
  template <> const int Statistic<unsigned>::vt_type = VT_COUNT_TYPE_UNSIGNED;
  template <> const int Statistic<uint64_t>::vt_type = VT_COUNT_TYPE_UNSIGNED;
  template <> const int Statistic<double>::vt_type = VT_COUNT_TYPE_DOUBLE;
  template <> const int Statistic<float>::vt_type = VT_COUNT_TYPE_FLOAT;
#endif
  
  std::vector<const StatisticBase *>& registered_stats() {
    static std::vector<const StatisticBase *> r;
    return r;
  }
  
  StatisticBase::StatisticBase(const char * name): name(name) {
    registered_stats().push_back(this);
    VLOG(1) << "registered <" << this->name << ">";
  }
  
  void PrintStatistics(std::ostream& out) {
    std::ostringstream o;
    o << "STATS{\n";
    for (const StatisticBase*& s : registered_stats()) {
      // skip printing "," before first one
      if (&s-&registered_stats()[0] != 0) { o << ",\n"; }
      
      o << "  " << *s;
    }
    o << "\n}STATS";
    out << o.str() << std::endl;
  }
  
} // namespace Grappa