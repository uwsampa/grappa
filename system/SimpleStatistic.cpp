#include "SimpleStatistic.hpp"
#include "SimpleStatisticImpl.hpp"

namespace Grappa {

#ifdef VTRACE_SAMPLED
  template <> inline void SimpleStatistic<int>::vt_sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter, value);
  }
  template <> inline void SimpleStatistic<int64_t>::vt_sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter, value);
  }
  template <> inline void SimpleStatistic<unsigned>::vt_sample() const {
    VT_COUNT_UNSIGNED_VAL(vt_counter, value);
  }
  template <> inline void SimpleStatistic<uint64_t>::vt_sample() const {
    VT_COUNT_UNSIGNED_VAL(vt_counter, value);
  }
  template <> inline void SimpleStatistic<double>::vt_sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter, value);
  }
  template <> inline void SimpleStatistic<float>::vt_sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter, value);
  }
  
  template <> const int SimpleStatistic<int>::vt_type = VT_COUNT_TYPE_SIGNED;
  template <> const int SimpleStatistic<int64_t>::vt_type = VT_COUNT_TYPE_SIGNED;
  template <> const int SimpleStatistic<unsigned>::vt_type = VT_COUNT_TYPE_UNSIGNED;
  template <> const int SimpleStatistic<uint64_t>::vt_type = VT_COUNT_TYPE_UNSIGNED;
  template <> const int SimpleStatistic<double>::vt_type = VT_COUNT_TYPE_DOUBLE;
  template <> const int SimpleStatistic<float>::vt_type = VT_COUNT_TYPE_FLOAT;
#endif

  // force instantiation of merge_all()
  template void SimpleStatistic<int>::merge_all(impl::StatisticBase*);
  template void SimpleStatistic<int64_t>::merge_all(impl::StatisticBase*);
  template void SimpleStatistic<unsigned>::merge_all(impl::StatisticBase*);
  template void SimpleStatistic<uint64_t>::merge_all(impl::StatisticBase*);
  template void SimpleStatistic<double>::merge_all(impl::StatisticBase*);
  template void SimpleStatistic<float>::merge_all(impl::StatisticBase*);
}

