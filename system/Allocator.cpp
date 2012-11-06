
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

/// Generic buddy allocator. Used by GlobalAllocator to implement
/// global heap. 

#include "Allocator.hpp"


std::ostream& operator<<( std::ostream& o, const AllocatorChunk& chunk ) {
  return chunk.dump( o );
}

std::ostream& operator<<( std::ostream& o, const Allocator& a ) {
  return a.dump( o );
}

