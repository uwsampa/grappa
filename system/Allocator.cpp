
#include "Allocator.hpp"


std::ostream& operator<<( std::ostream& o, const AllocatorChunk& chunk ) {
  return chunk.dump( o );
}

std::ostream& operator<<( std::ostream& o, const Allocator& a ) {
  return a.dump( o );
}

