
#ifndef __ALLOC_H__
#define __ALLOC_H__

void * alloc_my_block( int node, int node_count, size_t total_size_bytes );
int free_my_block( int node, int node_count, size_t total_size_bytes, void * block );

#endif
