
#pragma once

#include "StatisticBase.hpp"
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

  namespace Statistics {
    void histogram_sample();
  }

  class HistogramStatistic : public impl::StatisticBase {
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
    
    HistogramStatistic(const char * name, int64_t nil_value, bool reg_new = true);
    
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
    
    virtual HistogramStatistic* clone() const {
      // (note: must do `reg_new`=false so we don't re-register this stat)
      return new HistogramStatistic(name, value_, false);
    }
    
    virtual void merge_all(impl::StatisticBase* static_stat_ptr) {
      HistogramStatistic* this_static = reinterpret_cast<HistogramStatistic*>(static_stat_ptr);
      
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
    
    inline HistogramStatistic& operator=(int64_t value) {
      this->value_ = value;
      return *this;
    }
    // </sugar>
  };
} // namespace Grappa
