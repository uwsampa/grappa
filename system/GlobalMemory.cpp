
#include "GlobalMemory.hpp"

GlobalMemory * global_memory = NULL;

size_t round_up_page_size( size_t s ) {
  // TODO: fix this for huge pages
  const size_t page_size = 1 << 12;

  if( s < page_size ) {
    s = page_size;
  }

  size_t diff = s % page_size;
  if( 0 != diff ) {
    s += page_size - s;
  }

  return s;
}

GlobalMemory::GlobalMemory( size_t total_size_bytes, 
                             void * base )
  : chunk_( round_up_page_size( total_size_bytes / SoftXMT_nodes() ),
            base )
  , allocator_( chunk_.global_pointer(), total_size_bytes )
{ 
  assert( !global_memory );
  global_memory = this;
}
