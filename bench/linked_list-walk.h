
#ifndef __WALK_H__
#define __WALK_H__

#include <smmintrin.h>

#include <stdint.h>
#include "linked_list-node.h"
#include "greenery/thread.h"

uint64_t singlewalk( node nodes[], uint64_t count );
uint64_t multiwalk( node* bases[], 
		    uint64_t concurrent_reads, 
		    uint64_t count );
uint64_t walk( thread* me, node* bases[], uint64_t count, int num_refs, int start_index );

#endif
