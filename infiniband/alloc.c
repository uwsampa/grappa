

#include "debug.h"
#include "alloc.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
//#include <hugetlbfs.h>

void * alloc_my_block( int node, int node_count, size_t total_size_bytes, size_t * my_size_bytes ) {
  *my_size_bytes = total_size_bytes / node_count;

//  void * block = get_huge_pages( *my_size_bytes, GHP_DEFAULT );
  void * block = malloc( *my_size_bytes );
  ASSERT_NZ( block );
  memset( block, 0, *my_size_bytes );
  return block;
}


void * alloc_my_block_mmap( int node, int node_count, size_t total_size_bytes, size_t * my_size_bytes ) {
  *my_size_bytes = total_size_bytes / node_count;

  return malloc( *my_size_bytes );

  void * base = (void *) 0x0000000100000000L;
  void * my_base = base + node * *my_size_bytes;

  void * block = (void *) mmap( my_base,
				*my_size_bytes,
				PROT_WRITE | PROT_READ,
				//MAP_FIXED | MAP_SHARED | MAP_ANONYMOUS | MAP_LOCKED | MAP_NORESERVE | MAP_POPULATE,
				MAP_SHARED | MAP_ANONYMOUS,
				-1,
				(off_t) 0);

  assert( MAP_FAILED == block && "mmap failed" );

  return block;
}

void free_my_block( void * block ) {
//  free_huge_pages( block );
	malloc( block );
}

int free_my_block_malloc( int node, int node_count, size_t total_size_bytes, void * block ) {
  free( block );
  return 0;

  size_t my_size_bytes = total_size_bytes / node_count;
  return munmap( block, my_size_bytes );
}
