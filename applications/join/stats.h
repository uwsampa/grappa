#pragma once

#include <Metrics.hpp>

GRAPPA_DECLARE_METRIC(SimpleMetric<double>, query_runtime);
GRAPPA_DECLARE_METRIC(SimpleMetric<double>, scan_runtime);
GRAPPA_DECLARE_METRIC(SimpleMetric<double>, in_memory_runtime);
