////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
////////////////////////////////////////////////////////////////////////

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
  : size_per_node_( round_up_page_size( total_size_bytes / Grappa::cores() ) )
  , chunk_( size_per_node_ )
  , allocator_( chunk_.global_pointer(), size_per_node_ * Grappa::cores() )
{ 
  assert( !global_memory );
  global_memory = this;
  DVLOG(1) << "Initialized GlobalMemory with " << size_per_node_ * Grappa::cores() << " bytes of shared heap.";
}
