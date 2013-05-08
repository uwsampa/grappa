#include "GlobalVector.hpp"

GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, global_vector_push_ops, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, global_vector_push_msgs, 0);

DEFINE_string(flat_combining, "on", "Set flat combining mode: (currently just 'on' or 'off')");

DEFINE_uint64(global_vector_buffer, 1<<10, "Maximum number of elements that can be buffered");
