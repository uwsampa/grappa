#include "SummarizingStatistic.hpp"
#include "SummarizingStatisticImpl.hpp"

namespace Grappa {

  // force instantiation of merge_all()
  template void SummarizingStatistic<int>::merge_all(impl::StatisticBase*);
  template void SummarizingStatistic<int64_t>::merge_all(impl::StatisticBase*);
  template void SummarizingStatistic<unsigned>::merge_all(impl::StatisticBase*);
  template void SummarizingStatistic<uint64_t>::merge_all(impl::StatisticBase*);
  template void SummarizingStatistic<double>::merge_all(impl::StatisticBase*);
  template void SummarizingStatistic<float>::merge_all(impl::StatisticBase*);
}

