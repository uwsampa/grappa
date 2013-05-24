#include "GlobalHashSet.hpp"

// GRAPPA_DEFINE_STAT(MaxStatistic<uint64_t>, max_cell_length, 0);

GRAPPA_DEFINE_STAT(SummarizingStatistic<uint64_t>, cell_traversal_length, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<size_t>, hashset_insert_ops, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<size_t>, hashset_insert_msgs, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<size_t>, hashset_lookup_ops, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<size_t>, hashset_lookup_msgs, 0);
