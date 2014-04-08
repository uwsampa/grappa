#include "graphlab.hpp"

GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, iteration_time, 0);

DEFINE_int32(max_iterations, 1024, "Stop after this many iterations, no matter what.");


Reducer<int64_t,ReducerType::Add> ct;
