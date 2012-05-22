
#ifndef __GLOBAL_MEMORY_CHUNK_HPP__
#define __GLOBAL_MEMORY_CHUNK_HPP__

#include "Addressing.hpp"

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
