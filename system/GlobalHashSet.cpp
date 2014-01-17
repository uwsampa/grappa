#include "GlobalHashSet.hpp"
#include <SimpleMetricImpl.hpp>
// GRAPPA_DEFINE_METRIC(MaxMetric<uint64_t>, max_cell_length, 0);

GRAPPA_DEFINE_METRIC(SummarizingMetric<uint64_t>, cell_traversal_length, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<size_t>, hashset_insert_ops, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<size_t>, hashset_insert_msgs, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<size_t>, hashset_lookup_ops, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<size_t>, hashset_lookup_msgs, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<size_t>, hashset_matched_lookups, 0);
