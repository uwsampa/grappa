
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "GlobalMemory.hpp"

GlobalMemory * global_memory = NULL;

/// round up address to 4KB page alignment
/// TODO: why did I do it this way?
size_t round_up_page_size( size_t s ) {
  // TODO: fix this for huge pages
  const size_t page_size = 1 << 12;
  size_t new_s = s;

  if( s < page_size ) {
    new_s = page_size;
  } else {
    size_t diff = s % page_size;
    if( 0 != diff ) {
      new_s = s + page_size - diff;
    }
  }

  DVLOG(5) << "GlobalMemory rounding up from " << s << " to " << new_s;
  return new_s;
}

/// Construct local aspect of global memory
GlobalMemory::GlobalMemory( size_t total_size_bytes )
  : size_per_node_( round_up_page_size( total_size_bytes / SoftXMT_nodes() ) )
  , chunk_( size_per_node_ )
  , allocator_( chunk_.global_pointer(), size_per_node_ * SoftXMT_nodes() )
{ 
  assert( !global_memory );
  global_memory = this;
  DVLOG(1) << "Initialized GlobalMemory with " << size_per_node_ * SoftXMT_nodes() << " bytes of shared heap.";
}
