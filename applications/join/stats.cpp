#include "stats.h"

GRAPPA_DEFINE_METRIC(SimpleMetric<double>, query_runtime, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, scan_runtime, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, in_memory_runtime,0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, init_runtime,0);
