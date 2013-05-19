#include "GlobalHashSet.hpp"

// GRAPPA_DEFINE_STAT(MaxStatistic<uint64_t>, max_cell_length, 0);

GRAPPA_DEFINE_STAT(SummarizingStatistic<uint64_t>, cell_traversal_length, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<size_t>, hash_set_inserts_flattened, 0);
