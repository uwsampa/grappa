
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef __GLOBAL_MEMORY_CHUNK_HPP__
#define __GLOBAL_MEMORY_CHUNK_HPP__

#include "Addressing.hpp"

/// One processes' chunk of the global memory.
class GlobalMemoryChunk
{
private:
  key_t shm_key_;
  int shm_id_;
  size_t size_;
  void * base_;
  void * memory_;

public:
  GlobalMemoryChunk( size_t size );
  ~GlobalMemoryChunk();

  void * local_pointer() {
    return memory_;
  }

  GlobalAddress< void > global_pointer()  {
    return make_linear( memory_ );
  }
};

#endif
