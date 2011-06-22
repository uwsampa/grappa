
#include <stdint.h>
#include "linked_list-node.h"

#include "thread.h"
#include "SplitPhase.hpp"


uint64_t walk_prefetch_switch( thread* me, node* bases[], uint64_t count, int num_refs, int start_index );

uint64_t walk_split_phase( thread* me, SplitPhase* sp, node* bases[], uint64_t count, int num_refs);

uint64_t walk( node* bases[], uint64_t count, int num_refs, int start_index );

