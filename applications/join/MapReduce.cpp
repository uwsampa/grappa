#include "MapReduce.hpp"

namespace MapReduce {
Grappa::GlobalCompletionEvent default_mr_gce;
}

GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, mr_mapping_runtime, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, mr_combining_runtime, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, mr_reducing_runtime, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, mr_reallocation_runtime, 0);
