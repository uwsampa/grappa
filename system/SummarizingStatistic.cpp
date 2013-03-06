#include "SummarizingStatistic.hpp"
#include "SummarizingStatisticImpl.hpp"

namespace Grappa {

#ifdef VTRACE_SAMPLED
  template <> inline void SummarizingStatistic<int>::vt_sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter, value);
  }
  template <> inline void SummarizingStatistic<int64_t>::vt_sample() const {
    VT_COUNT_SIGNED_VAL(vt_counter, value);
  }
  template <> inline void SummarizingStatistic<unsigned>::vt_sample() const {
    VT_COUNT_UNSIGNED_VAL(vt_counter, value);
  }
  template <> inline void SummarizingStatistic<uint64_t>::vt_sample() const {
    VT_COUNT_UNSIGNED_VAL(vt_counter, value);
  }
  template <> inline void SummarizingStatistic<double>::vt_sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter, value);
  }
  template <> inline void SummarizingStatistic<float>::vt_sample() const {
    VT_COUNT_DOUBLE_VAL(vt_counter, value);
  }
  
  template <> const int SummarizingStatistic<int>::vt_type = VT_COUNT_TYPE_SIGNED;
  template <> const int SummarizingStatistic<int64_t>::vt_type = VT_COUNT_TYPE_SIGNED;
  template <> const int SummarizingStatistic<unsigned>::vt_type = VT_COUNT_TYPE_UNSIGNED;
  template <> const int SummarizingStatistic<uint64_t>::vt_type = VT_COUNT_TYPE_UNSIGNED;
  template <> const int SummarizingStatistic<double>::vt_type = VT_COUNT_TYPE_DOUBLE;
  template <> const int SummarizingStatistic<float>::vt_type = VT_COUNT_TYPE_FLOAT;
#endif

  // force instantiation of merge_all()
  template void SummarizingStatistic<int>::merge_all(impl::StatisticBase*);
  template void SummarizingStatistic<int64_t>::merge_all(impl::StatisticBase*);
  template void SummarizingStatistic<unsigned>::merge_all(impl::StatisticBase*);
  template void SummarizingStatistic<uint64_t>::merge_all(impl::StatisticBase*);
  template void SummarizingStatistic<double>::merge_all(impl::StatisticBase*);
  template void SummarizingStatistic<float>::merge_all(impl::StatisticBase*);
}

