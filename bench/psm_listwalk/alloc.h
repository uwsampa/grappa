
#ifndef __ALLOC_H__
#define __ALLOC_H__

void allocate_heap(uint64_t size, int num_threads, int num_lists, node*** bases, node** nodes);

node** allocate_heap2(uint64_t size, int num_threads, int num_lists, node** nodes);

#endif
