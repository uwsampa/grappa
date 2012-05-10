
#ifndef __GLOBAL_MEMORY_HPP__
#define __GLOBAL_MEMORY_HPP__

#include <glog/logging.h>
#include "SoftXMT.hpp"

#include "GlobalMemoryChunk.hpp"
#include "GlobalAllocator.hpp"

class GlobalMemory
{
private:
  size_t size_per_node_;
  GlobalMemoryChunk chunk_;
  GlobalAllocator allocator_;

public:
  GlobalMemory( size_t total_size_bytes );

};

// TODO: we're using an allocator pointer instead of this for stuff
extern GlobalMemory * global_memory;

#endif
