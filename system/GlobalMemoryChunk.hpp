
#ifndef __GLOBAL_MEMORY_CHUNK_HPP__
#define __GLOBAL_MEMORY_CHUNK_HPP__

#include <glog/logging.h>

extern "C" {
#include <sys/mman.h>
}

#include "Addressing.hpp"


class GlobalMemoryChunk
{
private:
  size_t size_;
  void * base_;
  void * memory_;

public:
  GlobalMemoryChunk( size_t size, 
                     void * base = reinterpret_cast< void* >( 0x0000123400000000L ) )
    : size_( size )
    , base_( base )
    , memory_( 0 )
  {
    memory_ = static_cast< void * > ( mmap( base_, size_, 
                                            PROT_READ | PROT_WRITE, 
                                            MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, 
                                            0, 0 ) );
    CHECK_EQ( memory_, base ) << "GlobalMemoryChunk allocation didn't happen at specified base address";
  }

  ~GlobalMemoryChunk() 
  {
    munmap( base_, size_ );
  }

  void * local_pointer()
  {
    return memory_;
  }

  GlobalAddress< void > global_pointer()
  {
    return make_linear( memory_ );
  }
};

#endif
