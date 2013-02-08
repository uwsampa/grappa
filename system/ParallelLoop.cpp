#include "ParallelLoop.hpp"
#include "CompletionEvent.hpp"

DEFINE_int64(loop_threshold, 1, "threshold for how small a group of iterations should be to perform them serially");

namespace Grappa {
  CompletionEvent local_ce;
}
