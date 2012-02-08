#ifndef LINKED_LIST_WALK_H_
#define LINKED_LIST_WALK_H_

#include <stdint.h>

#include "thread.h"
#include "SplitPhase.hpp"

uint64_t walk_split_phase(thread* me, SplitPhase* sp, global_array* vertices, uint64_t head, uint64_t listsize, uint64_t num_lists);


#endif
