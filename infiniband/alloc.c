
#include <assert.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "alloc.h"

void * alloc_my_block( int node, int node_count, size_t total_size_bytes ) {
  size_t my_size_bytes = total_size_bytes / node_count;

  return malloc(my_size_bytes);

  void * base = (void *) 0x0000000100000000L;
  void * my_base = base + node * my_size_bytes;

  void * block = (void *) mmap( NULL,
				my_size_bytes,
				PROT_WRITE | PROT_READ,
				//MAP_FIXED | MAP_SHARED | MAP_ANONYMOUS | MAP_LOCKED | MAP_NORESERVE | MAP_POPULATE,
				MAP_SHARED | MAP_ANONYMOUS,
				-1,
				(off_t) 0);

  assert( MAP_FAILED == block && "mmap failed" );

  return block;
}

int free_my_block( int node, int node_count, size_t total_size_bytes, void * block ) {
  free( block );
  return 0;

  size_t my_size_bytes = total_size_bytes / node_count;
  return munmap( block, my_size_bytes );
}
