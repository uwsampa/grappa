#ifndef LINKED_LIST_WALK_H_
#define LINKED_LIST_WALK_H_

#include <stdint.h>

#include "thread.h"
#include "SplitPhase.hpp"

int walk_split_phase(thread* me, SplitPhase* sp, int64_t base, uint64_t listsize, uint64_t num_lists);


#endif
