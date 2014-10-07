#include "stats.h"

GRAPPA_DEFINE_METRIC(SimpleMetric<double>, query_runtime, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, scan_runtime, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, in_memory_runtime,0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, init_runtime,0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, join_coarse_result_count,0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, emit_count,0);

