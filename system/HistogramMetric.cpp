#include "HistogramMetric.hpp"

namespace Grappa {

  namespace impl {
    std::vector<HistogramMetric*>& histogram_stats() {        
      static std::vector<HistogramMetric*> r;
      return r;
    }
  }

  namespace Metrics {
    
    void histogram_sample() {
      for (HistogramMetric * h : Grappa::impl::histogram_stats()) {
        DVLOG(5) << "sampling...";
        h->sample();
      }
    }
  
  }
  HistogramMetric::HistogramMetric(const char * name, int64_t nil_value, bool reg_new)
    : impl::MetricBase(name, reg_new)
    , nil_value(nil_value)
    , value_(nil_value)
    , log_initialized(false)
  {
  #ifdef HISTOGRAM_SAMPLED
    impl::histogram_stats().push_back(this);
  #endif //HISint64_tOGRAM_SAMPLED
  }
  
}