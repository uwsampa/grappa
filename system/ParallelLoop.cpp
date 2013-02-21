#include "ParallelLoop.hpp"
#include "CompletionEvent.hpp"
#include "GlobalCompletionEvent.hpp"

DEFINE_int64(loop_threshold, 16, "threshold for how small a group of iterations should be to perform them serially");

namespace Grappa {
  CompletionEvent local_ce;
  GlobalCompletionEvent local_gce;
}
