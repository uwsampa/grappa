
#ifndef __ALLOC_H__
#define __ALLOC_H__

#include "linked_list-node.h"


node** allocate_page(uint64_t size, int num_threads, int num_lists_per_thread);
node** allocate_heap(uint64_t size, int num_threads, int num_lists);
node** allocate_2numa_heap(uint64_t size, int num_threads, int num_lists,
                          node** node0_low, node** node0_high,
                          node** node1_low, node** node1_high);




#endif
