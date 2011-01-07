
#include <stdint.h>
#include "linked_list-node.h"

#include "thread.h"

uint64_t walk_prefetch_switch( thread* me, node* bases[], uint64_t count, int num_refs, int start_index );

uint64_t walk( node* bases[], uint64_t count, int num_refs, int start_index );

