#include "ForkJoin.hpp"

DEFINE_int64(max_forkjoin_threads_per_node, 1024, "maximum number of threads to spawn for a fork-join region");

LocalTaskJoiner ljoin;
