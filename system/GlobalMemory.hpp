
#ifndef __GLOBAL_MEMORY_HPP__
#define __GLOBAL_MEMORY_HPP__

#include <glog/logging.h>
#include "Addressing.hpp"

template< typename T = void >
class GlobalMemory 
{
private:
  size_t size_;
  void * base_;
  T * memory_;

public:
  GlobalMemory( size_t size, void * base = reinterpret_cast< void* >( 0x0000123400000000L ) )
    : size_( size )
    , base_( base )
    , memory_( 0 )
  {
    memory_ = static_cast< T * > ( mmap( base_, size_ * sizeof(T), 
                                         PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, 
                                         0, 0 ) );
    CHECK_EQ( memory_, base ) << "GlobalMemory allocation didn't happen at specified base address";
  }

  ~GlobalMemory() 
  {
    munmap( base_, size_ * sizeof(T) );
  }

  T * local_pointer()
  {
    return memory_;
  }

  GlobalAddress< T > global_pointer()
  {
    return make_linear( memory_ );
  }
};

#endif
