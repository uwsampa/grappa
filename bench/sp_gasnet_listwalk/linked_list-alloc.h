#ifndef _LINKED_LIST_ALLOC_
#define _LINKED_LIST_ALLOC_

#include <stdint.h>
#include "global_array.h"

void allocate_lists(global_array* vertices, uint64_t num_vertices, uint64_t startIndex, uint64_t endIndex, int myrank, uint64_t* localHeads, uint64_t num_lists_per_node, uint64_t num_vertices_per_list);

#endif
