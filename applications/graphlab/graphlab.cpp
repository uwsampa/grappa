////////////////////////////////////////////////////////////////////////
/// GraphLab is an API and runtime system for graph-parallel computation.
/// This is a rough prototype implementation of the programming model to
/// demonstrate using Grappa as a platform for other models.
/// More information on the actual GraphLab system can be found at:
/// graphlab.org.
////////////////////////////////////////////////////////////////////////

#include "graphlab.hpp"

GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, iteration_time, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<int>, core_set_size, 0);

DEFINE_int32(max_iterations, 1024, "Stop after this many iterations, no matter what.");
