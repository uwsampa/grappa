
#ifndef __WALK_H__
#define __WALK_H__

#include <smmintrin.h>

#include <stdint.h>
#include "linked_list-node.h"
#include "greenery/thread.h"

uint64_t walk_prefetch( thread* me, node* bases[], uint64_t count, int num_refs, int start_index );
uint64_t walk_raw( thread* me, node* bases[], uint64_t count, int num_refs, int start_index );

#endif
