
#ifndef __ALLOC_H__
#define __ALLOC_H__

#include "node.h"


node** allocate_page(uint64_t size, int num_threads, int num_lists_per_thread);
node** allocate_heap(uint64_t size, int num_threads, int num_lists);

#endif
