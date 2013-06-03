#include "GlobalHashSet.hpp"

GRAPPA_DEFINE_STAT(SimpleStatistic<size_t>, hashmap_insert_ops, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<size_t>, hashmap_insert_msgs, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<size_t>, hashmap_lookup_ops, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<size_t>, hashmap_lookup_msgs, 0);
