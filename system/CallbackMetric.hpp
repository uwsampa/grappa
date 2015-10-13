////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////

#pragma once

#include "MetricBase.hpp"
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

  /// Metric that uses a callback to determine the value at sample time.
  /// Used for sampling state when the state is not declared within modifiable code.
  /// An example would be keeping track of the length of a std::vector.
  /// One would create a CallbackMetric with the callback vector.size();

  template<typename T>
    class CallbackMetric : public impl::MetricBase {
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
    CallbackMetric(const char * name, CallbackFn f, bool reg_new = true): f_(f), is_merged_(false), impl::MetricBase(name, reg_new) {
#ifdef VTRACE_SAMPLED
        if (CallbackMetric::vt_type == -1) {
          LOG(ERROR) << "warning: VTrace sampling unsupported for this type of SimpleMetric.";
        } else {
          vt_counter = VT_COUNT_DEF(name, name, CallbackMetric::vt_type, VT_COUNT_DEFGROUP);
        }
#endif
    }
    
    /// Get the current value
    inline T value() const {
      return f_();
    }

    void start_merging() {
      merging_value_ = 0;
      is_merged_ = true;
    }

    /// Note this will only clone its behavior
    virtual CallbackMetric<T>* clone() const {
      return new CallbackMetric<T>( name, f_, false);
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
      // vt_sample() specialized for supported tracing types in Metrics.cpp
      vt_sample();
#endif
    }
    
    virtual void merge_all(impl::MetricBase* static_stat_ptr);
  
    };
    /// @}
} // namespace Grappa
