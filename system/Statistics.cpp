
#include "Statistics.hpp"
#include <vector>
#include <iostream>
#include <sstream>

namespace Grappa {

#ifdef VTRACE_SAMPLED
  template <> void Statistic<int>::sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter, value);
  }
  template <> void Statistic<double>::sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter, value);
  }
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