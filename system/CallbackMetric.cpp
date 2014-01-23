
#include "CallbackMetric.hpp"
#include "CallbackMetricImpl.hpp"

namespace Grappa {

#ifdef VTRACE_SAMPLED
  template <> void CallbackMetric<int>::vt_sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter, value());
  }
  template <> void CallbackMetric<int64_t>::vt_sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter, value());
  }
  template <> void CallbackMetric<unsigned>::vt_sample() const {
    VT_COUNT_UNSIGNED_VAL(vt_counter, value());
  }
  template <> void CallbackMetric<uint64_t>::vt_sample() const {
    VT_COUNT_UNSIGNED_VAL(vt_counter, value());
  }
  template <> void CallbackMetric<double>::vt_sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter, value());
  }
  template <> void CallbackMetric<float>::vt_sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter, value());
  }
  
  template <> const int CallbackMetric<int>::vt_type = VT_COUNT_TYPE_SIGNED;
  template <> const int CallbackMetric<int64_t>::vt_type = VT_COUNT_TYPE_SIGNED;
  template <> const int CallbackMetric<unsigned>::vt_type = VT_COUNT_TYPE_UNSIGNED;
  template <> const int CallbackMetric<uint64_t>::vt_type = VT_COUNT_TYPE_UNSIGNED;
  template <> const int CallbackMetric<double>::vt_type = VT_COUNT_TYPE_DOUBLE;
  template <> const int CallbackMetric<float>::vt_type = VT_COUNT_TYPE_FLOAT;
#endif

  // force instantiation of merge_all()
  template void CallbackMetric<int>::merge_all(impl::MetricBase*);
  template void CallbackMetric<int64_t>::merge_all(impl::MetricBase*);
  template void CallbackMetric<unsigned>::merge_all(impl::MetricBase*);
  template void CallbackMetric<uint64_t>::merge_all(impl::MetricBase*);
  template void CallbackMetric<double>::merge_all(impl::MetricBase*);
  template void CallbackMetric<float>::merge_all(impl::MetricBase*);
}

