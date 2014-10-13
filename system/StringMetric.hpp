////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
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
      // this->json(VLOG(4) << "cloned is ");
      // x.json(VLOG(4) << "clone is ");
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
