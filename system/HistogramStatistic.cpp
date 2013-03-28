#include "HistogramStatistic.hpp"

namespace Grappa {

  namespace Statistics {
  
    namespace impl {
      std::vector<HistogramStatistic*>& histogram_stats() {        
        static std::vector<HistogramStatistic*> r;
        return r;
      }
    }
  
    void histogram_sample() {
      for (HistogramStatistic * h : Grappa::Statistics::impl::histogram_stats()) {
        VLOG(1) << "sampling...";
        h->sample();
      }
    }
  
  }
  HistogramStatistic::HistogramStatistic(const char * name, int64_t nil_value, bool reg_new)
    : impl::StatisticBase(name, reg_new)
    , nil_value(nil_value)
    , value_(nil_value)
    , log_initialized(false)
  {
  #ifdef HISTOGRAM_SAMPLED
    Statistics::impl::histogram_stats().push_back(this);
  #endif //HISint64_tOGRAM_SAMPLED
  }
  
}