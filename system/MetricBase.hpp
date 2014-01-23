
#pragma once

#include <ostream>

namespace Grappa {
  namespace impl {
  
    class MetricBase {
    protected:
      const char * name;
      
    public:
      /// registers stat with global stats vector by default (reg_new=false to disable)
      MetricBase(const char * name, bool reg_new = true);
      
      /// prints as a JSON entry
      virtual std::ostream& json(std::ostream&) const = 0;
      
      /// reinitialize counter values, etc (so we can collect stats on specific region)
      virtual void reset() = 0;
      
      /// periodic sample (VTrace sampling triggered by GPerf stuff)
      virtual void sample() = 0;
      
      /// communicate with each cores' stat at the given pointer and aggregate them into
      /// this stat object
      virtual void merge_all(MetricBase* static_stat_ptr) = 0;
      
      /// create new copy of the class of the right instance (needed so we can create new copies of stats from their MetricBase pointer
      virtual MetricBase* clone() const = 0;
    };
    
  }
}
