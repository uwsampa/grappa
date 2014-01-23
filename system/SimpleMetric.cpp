#include "SimpleMetric.hpp"
#include "SimpleMetricImpl.hpp"

namespace Grappa {

#ifdef VTRACE_SAMPLED
  template <> void SimpleMetric<int>::vt_sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter, value_);
  }
  template <> void SimpleMetric<int64_t>::vt_sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter, value_);
  }
  template <> void SimpleMetric<unsigned>::vt_sample() const {
    VT_COUNT_UNSIGNED_VAL(vt_counter, value_);
  }
  template <> void SimpleMetric<uint64_t>::vt_sample() const {
    VT_COUNT_UNSIGNED_VAL(vt_counter, value_);
  }
  template <> void SimpleMetric<double>::vt_sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter, value_);
  }
  template <> void SimpleMetric<float>::vt_sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter, value_);
  }
  
  template <> const int SimpleMetric<int>::vt_type = VT_COUNT_TYPE_SIGNED;
  template <> const int SimpleMetric<int64_t>::vt_type = VT_COUNT_TYPE_SIGNED;
  template <> const int SimpleMetric<unsigned>::vt_type = VT_COUNT_TYPE_UNSIGNED;
  template <> const int SimpleMetric<uint64_t>::vt_type = VT_COUNT_TYPE_UNSIGNED;
  template <> const int SimpleMetric<double>::vt_type = VT_COUNT_TYPE_DOUBLE;
  template <> const int SimpleMetric<float>::vt_type = VT_COUNT_TYPE_FLOAT;
#endif

  // force instantiation of merge_all()
  template void SimpleMetric<int>::merge_all(impl::MetricBase*);
  template void SimpleMetric<int64_t>::merge_all(impl::MetricBase*);
  template void SimpleMetric<unsigned>::merge_all(impl::MetricBase*);
  template void SimpleMetric<uint64_t>::merge_all(impl::MetricBase*);
  template void SimpleMetric<double>::merge_all(impl::MetricBase*);
  template void SimpleMetric<float>::merge_all(impl::MetricBase*);
}

