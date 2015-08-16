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
#include <fstream>
#include "Communicator.hpp"

#ifdef VTRACE
#include <vt_user.h>
#endif

#ifdef GOOGLE_PROFILER
#include <gperftools/profiler.h>
#endif


namespace Grappa {

  namespace Metrics {
    void histogram_sample();
  }

  class HistogramMetric : public impl::MetricBase {
  protected:
    int64_t nil_value;
    int64_t value_;
    bool log_initialized;
    
    std::ofstream log;
    
#ifdef VTRACE_SAMPLED
    unsigned vt_counter;
    static const int vt_type;
    
    void vt_sample() const;
#endif
    
  public:
    
    HistogramMetric(const char * name, int64_t nil_value, bool reg_new = true);
    
    virtual std::ostream& json(std::ostream& o) const {
      o << '"' << name << "\": " << "\"(see histogram)\"";
      return o;
    }
    
    virtual void reset() {
      DVLOG(5) << "resetting histogram";
      if (!log_initialized) {
        log_initialized = true;
        char * jobid = getenv("SLURM_JOB_ID");
        char fname[256]; sprintf(fname, "histogram.%s/%s.%d.out", jobid, name, mycore());
        log.open(fname, std::ios::out | std::ios_base::binary);
        DVLOG(5) << "opened " << fname;
      }
      if (log_initialized) {
        log.clear();
        log.seekp(0, std::ios::beg);
      }
      value_ = nil_value;
    }
    
    virtual void sample() {
      DVLOG(5) << "sampling " << name;
      if (value_ != nil_value) log.write((char*)&value_, sizeof(&value_));
      value_ = nil_value;
    }
    
    virtual HistogramMetric* clone() const {
      // (note: must do `reg_new`=false so we don't re-register this stat)
      return new HistogramMetric(name, value_, false);
    }
    
    virtual void merge_all(impl::MetricBase* static_stat_ptr) {
      HistogramMetric* this_static = reinterpret_cast<HistogramMetric*>(static_stat_ptr);
      
      this_static->value_ = 0; // don't care about merging these
#ifdef HISTOGRAM_SAMPLED
      // call_on_all_cores([this]{ if (log_initialized) log.flush(); });
#endif
    }

    /// Get the current value
    inline int64_t value() const { return value_; }
    
    inline void set(const int64_t& o) { value_ = o; }
    
    // <sugar>    
    // allow casting as just the value
    inline operator int64_t() const { return value_; }
    
    inline HistogramMetric& operator=(int64_t value) {
      this->value_ = value;
      return *this;
    }
    // </sugar>
  };
} // namespace Grappa
