#include "GlobalVector.hpp"

GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, global_vector_push_ops, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, global_vector_push_msgs, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, global_vector_pop_ops, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, global_vector_pop_msgs, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, global_vector_deq_ops, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, global_vector_deq_msgs, 0);
GRAPPA_DEFINE_STAT(SummarizingStatistic<double>, global_vector_push_latency, 0);
GRAPPA_DEFINE_STAT(SummarizingStatistic<double>, global_vector_pop_latency, 0);
GRAPPA_DEFINE_STAT(SummarizingStatistic<double>, global_vector_deq_latency, 0);
