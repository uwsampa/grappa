
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef __GLOBAL_MEMORY_HPP__
#define __GLOBAL_MEMORY_HPP__

#include <glog/logging.h>
#include "Grappa.hpp"

#include "GlobalMemoryChunk.hpp"
#include "GlobalAllocator.hpp"

/// Representative of global memory in each process.
class GlobalMemory
{
private:
  size_t size_per_node_;
  GlobalMemoryChunk chunk_;
  GlobalAllocator allocator_;

public:
  GlobalMemory( size_t total_size_bytes );

};

/// Global memory pointer
/// TODO: remove. we're using an allocator pointer instead of this for stuff
extern GlobalMemory * global_memory;

#endif
