#include "GlobalVector.hpp"

GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, global_vector_push_ops, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, global_vector_push_msgs, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, global_vector_pop_ops, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, global_vector_pop_msgs, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, global_vector_deq_ops, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, global_vector_deq_msgs, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, global_vector_matched_pushes, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, global_vector_matched_pops, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, global_vector_push_latency, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, global_vector_pop_latency, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, global_vector_deq_latency, 0);

GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, global_vector_master_combined, 0);
