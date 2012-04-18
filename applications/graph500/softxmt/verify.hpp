#include "Addressing.hpp"
#include "common.h"

void compute_levels(GlobalAddress<int64_t> level, int64_t nv, GlobalAddress<int64_t> bfs_tree, int64_t root);

int64_t verify_bfs_tree(GlobalAddress<int64_t> bfs_tree, int64_t max_bfsvtx, int64_t root, tuple_graph * tg);
