#pragma once

#include <Metrics.hpp>

GRAPPA_DECLARE_METRIC(SimpleMetric<double>, query_runtime);
GRAPPA_DECLARE_METRIC(SimpleMetric<double>, scan_runtime);
GRAPPA_DECLARE_METRIC(SimpleMetric<double>, in_memory_runtime);
GRAPPA_DECLARE_METRIC(SimpleMetric<double>, init_runtime);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, join_coarse_result_count);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, emit_count);
