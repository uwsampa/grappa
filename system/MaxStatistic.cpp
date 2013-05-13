#include "MaxStatistic.hpp"
#include "MaxStatisticImpl.hpp"

namespace Grappa {

#ifdef VTRACE_SAMPLED
  template <> void MaxStatistic<int>::vt_sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter, value_);
  }
  template <> void MaxStatistic<int64_t>::vt_sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter, value_);
  }
  template <> void MaxStatistic<unsigned>::vt_sample() const {
    VT_COUNT_UNSIGNED_VAL(vt_counter, value_);
  }
  template <> void MaxStatistic<uint64_t>::vt_sample() const {
    VT_COUNT_UNSIGNED_VAL(vt_counter, value_);
  }
  template <> void MaxStatistic<double>::vt_sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter, value_);
  }
  template <> void MaxStatistic<float>::vt_sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter, value_);
  }
  
  template <> const int MaxStatistic<int>::vt_type = VT_COUNT_TYPE_SIGNED;
  template <> const int MaxStatistic<int64_t>::vt_type = VT_COUNT_TYPE_SIGNED;
  template <> const int MaxStatistic<unsigned>::vt_type = VT_COUNT_TYPE_UNSIGNED;
  template <> const int MaxStatistic<uint64_t>::vt_type = VT_COUNT_TYPE_UNSIGNED;
  template <> const int MaxStatistic<double>::vt_type = VT_COUNT_TYPE_DOUBLE;
  template <> const int MaxStatistic<float>::vt_type = VT_COUNT_TYPE_FLOAT;
#endif

  // force instantiation of merge_all()
  template void MaxStatistic<int>::merge_all(impl::StatisticBase*);
  template void MaxStatistic<int64_t>::merge_all(impl::StatisticBase*);
  template void MaxStatistic<unsigned>::merge_all(impl::StatisticBase*);
  template void MaxStatistic<uint64_t>::merge_all(impl::StatisticBase*);
  template void MaxStatistic<double>::merge_all(impl::StatisticBase*);
  template void MaxStatistic<float>::merge_all(impl::StatisticBase*);
}

