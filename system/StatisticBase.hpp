
#pragma once

#include <ostream>

namespace Grappa {

  class StatisticBase {
  protected:
    const char * name;
    
  public:
    /// registers stat with global stats vector by default (reg_new=false to disable)
    StatisticBase(const char * name, bool reg_new = true);
    
    /// prints as a JSON entry
    virtual std::ostream& json(std::ostream&) const = 0;
    
    /// periodic sample (VTrace sampling triggered by GPerf stuff)
    virtual void sample() const = 0;
    
    virtual void merge_all(StatisticBase* static_stat_ptr) = 0;
    
    virtual StatisticBase* clone() const = 0;
  };

}
