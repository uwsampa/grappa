#include "GlobalCompletionEvent.hpp"

DEFINE_bool(flatten_completions, true, "Flatten GlobalCompletionEvents.");

GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, gce_total_remote_completions, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, gce_completions_sent, 0);
