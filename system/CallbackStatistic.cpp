
#include "CallbackStatistic.hpp"
#include "CallbackStatisticImpl.hpp"

namespace Grappa {

#ifdef VTRACE_SAMPLED
  template <> void CallbackStatistic<int>::vt_sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter, value());
  }
  template <> void CallbackStatistic<int64_t>::vt_sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter, value());
  }
  template <> void CallbackStatistic<unsigned>::vt_sample() const {
    VT_COUNT_UNSIGNED_VAL(vt_counter, value());
  }
  template <> void CallbackStatistic<uint64_t>::vt_sample() const {
    VT_COUNT_UNSIGNED_VAL(vt_counter, value());
  }
  template <> void CallbackStatistic<double>::vt_sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter, value());
  }
  template <> void CallbackStatistic<float>::vt_sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter, value());
  }
  
  template <> const int CallbackStatistic<int>::vt_type = VT_COUNT_TYPE_SIGNED;
  template <> const int CallbackStatistic<int64_t>::vt_type = VT_COUNT_TYPE_SIGNED;
  template <> const int CallbackStatistic<unsigned>::vt_type = VT_COUNT_TYPE_UNSIGNED;
  template <> const int CallbackStatistic<uint64_t>::vt_type = VT_COUNT_TYPE_UNSIGNED;
  template <> const int CallbackStatistic<double>::vt_type = VT_COUNT_TYPE_DOUBLE;
  template <> const int CallbackStatistic<float>::vt_type = VT_COUNT_TYPE_FLOAT;
#endif

  // force instantiation of merge_all()
  template void CallbackStatistic<int>::merge_all(impl::StatisticBase*);
  template void CallbackStatistic<int64_t>::merge_all(impl::StatisticBase*);
  template void CallbackStatistic<unsigned>::merge_all(impl::StatisticBase*);
  template void CallbackStatistic<uint64_t>::merge_all(impl::StatisticBase*);
  template void CallbackStatistic<double>::merge_all(impl::StatisticBase*);
  template void CallbackStatistic<float>::merge_all(impl::StatisticBase*);
}

