#pragma once

#include "StatisticBase.hpp"
#include <glog/logging.h>

#ifdef VTRACE
#include <vt_user.h>
#endif

#ifdef GOOGLE_PROFILER
#include <gperftools/profiler.h>
#endif

#include <functional>

namespace Grappa {
  /// @addtogroup Utility
  /// @{

  /// Statistic that uses a callback to determine the value at sample time.
  /// Used for sampling state when the state is not declared within modifiable code.
  /// An example would be keeping track of the length of a std::vector.
  /// One would create a CallbackStatistic with the callback vector.size();

  template<typename T>
    class CallbackStatistic : public impl::StatisticBase {
      private:
        typedef T (*CallbackFn)(void);

        CallbackFn f_;

        // to support imperative merging
        // this statistic goes into value mode
        T merging_value_;
        bool is_merged_;


#ifdef VTRACE_SAMPLED
    unsigned vt_counter;
    static const int vt_type;
    
    void vt_sample() const;
#endif

      public:
    CallbackStatistic(const char * name, CallbackFn f, bool reg_new = true): f_(f), is_merged_(false), impl::StatisticBase(name, reg_new) {
#ifdef VTRACE_SAMPLED
        if (CallbackStatistic::vt_type == -1) {
          LOG(ERROR) << "warning: VTrace sampling unsupported for this type of SimpleStatistic.";
        } else {
          vt_counter = VT_COUNT_DEF(name, name, CallbackStatistic::vt_type, VT_COUNT_DEFGROUP);
        }
#endif
    }
    
    /// Get the current value
    inline T value() const {
      return f_();
    }

    bool start_merging() {
      merging_value_ = 0;
      is_merged_ = true;
    }

    /// Note this will only clone its behavior
    virtual CallbackStatistic<T>* clone() const {
      return new CallbackStatistic<T>( name, f_, false);
    }

    virtual std::ostream& json(std::ostream& o) const {
      if (is_merged_) {
        // if the target of a merge, then use that value
        o << '"' << name << "\": " << merging_value_;
        return o;
      } else {
        // otherwise use the regular callback value
        o << '"' << name << "\": " << value();
        return o;
      }
    }
    
    virtual void reset() {
      is_merged_ = false;
    }
    
    virtual void sample() {
#ifdef VTRACE_SAMPLED
      // vt_sample() specialized for supported tracing types in Statistics.cpp
      vt_sample();
#endif
    }
    
    virtual void merge_all(impl::StatisticBase* static_stat_ptr);
  
    };
    /// @}
} // namespace Grappa
