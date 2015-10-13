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
#include <string>
#include <cstdio>
#include <sstream>


namespace Grappa {
  /// @addtogroup Utility
  /// @{

  /// Metric that simply keeps track of a single string value over time.
  /// Typically used as a counter, but can also be used for sampling an instantaneous value.

  // Design note: We chose not to use template specialization 
  // on SimpleMetric because enough methods would have to be specialized that it isn't worth it
  class StringMetric : public impl::MetricBase {
  public:
    enum { max_string_size = 2048 };

    // utility for safe writing using max_string_size
    static void write_chars(char * dst, std::string newstr, std::string name="(anonymous)") {
      auto written = snprintf(dst, max_string_size, "%s", newstr.c_str());
      CHECK( written < max_string_size ) << "StringMetric " << name << " got assigned longer than max " << max_string_size;
      CHECK( written >= 0 ) << "StringMetric " << name << " assignment failure (code=" << written << ")";
    }

  private:
    void write_value(std::string newstr) {
      write_chars(this->value_, newstr, this->name);
    }

  protected:
    typedef std::function<std::string(void)> InitFn;
    std::string initial_value;
    char value_[max_string_size]; // store as a fixed size cstr so we can move it around
    InitFn initf_;
    
#ifdef VTRACE_SAMPLED
    unsigned vt_counter;
    static const int vt_type;
    
    void vt_sample() const;
#endif
    
  public:
    StringMetric(const char * name, std::string initial_value, bool reg_new = true):
        initial_value(initial_value), initf_(NULL), impl::MetricBase(name, reg_new) {
          write_value(initial_value);
#ifdef VTRACE_SAMPLED
        if (StringMetric::vt_type == -1) {
          LOG(ERROR) << "warning: VTrace sampling unsupported for this type of StringMetric.";
        } else {
          vt_counter = VT_COUNT_DEF(name, name, StringMetric::vt_type, VT_COUNT_DEFGROUP);
        }
#endif
    }
   
    
    virtual std::ostream& json(std::ostream& o) const {
      o << '"' << name << "\": " << value_;
      return o;
    }
    
    virtual void reset() {
      if (initf_ != NULL) {
        write_value(initf_());
      } else {
        write_value(initial_value);
      }
    }
    
    virtual void sample() {
#ifdef VTRACE_SAMPLED
      // vt_sample() specialized for supported tracing types in Metrics.cpp
      vt_sample();
#endif
    }
    
    virtual StringMetric* clone() const {
      // (note: must do `reg_new`=false so we don't re-register this stat)
      StringMetric x(name, std::string(value_), false);
#if DEBUG
      std::ostringstream o;
      this->json(o << "cloned is ");
      x.json(o << "clone is ");
      VLOG(4) << o.str();
#endif
      return new StringMetric(name, std::string(value_), false);
    }
    
    virtual void merge_all(impl::MetricBase* static_stat_ptr);

    /// Get the current value
    inline std::string value() const { return std::string(value_); }
    
    // <sugar>
    inline const StringMetric& operator+=(std::string appended) {
      auto newstr = std::string(value_) + appended;
      write_value(newstr);
      return *this;
    }
    
    // allow casting as just the value
    inline operator std::string() const { return std::string(value_); }
    
    inline StringMetric& operator=(std::string value) {
      write_value(value);
      return *this;
    }
    // </sugar>
  };
  
  /// @}
} // namespace Grappa
