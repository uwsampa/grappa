#include "MatchesDHT.hpp"

// for all hash tables
//GRAPPA_DEFINE_METRIC(MaxMetric<uint64_t>, max_cell_length, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, hash_tables_size, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<uint64_t>, hash_tables_lookup_steps, 0);

