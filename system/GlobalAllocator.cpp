
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "GlobalAllocator.hpp"


/// global GlobalAllocator pointer
GlobalAllocator * global_allocator = NULL;

/// dump
std::ostream& operator<<( std::ostream& o, const GlobalAllocator& a ) {
  return a.dump( o );
}


