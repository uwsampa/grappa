#pragma once

#include <glog/logging.h>

namespace Grappa {

  template<typename Base>
  class PoolAllocator {
    char * buffer;
    size_t buffer_size;
    size_t allocated;
  protected:
    Base* allocate(size_t sz) {
      LOG(ERROR) << "allocating " << sz;
      Base* p = reinterpret_cast<Base*>(buffer+allocated);
      allocated += sz;
      
      // TODO: implement heap allocation fallback
      CHECK(allocated <= buffer_size) << "PoolAllocator overflow: tried allocating " << sz << ", already allocated " << allocated << " of " << buffer_size;
      return p;
    }
    
  public:
    PoolAllocator(char * buffer, size_t buffer_size): buffer(buffer), buffer_size(buffer_size), allocated(0) {}
    
    virtual ~PoolAllocator() {
      // call destructors of everything in PoolAllocator
      iterate([](Base* bp){ bp->~Base(); });
    }
    
    /// Takes a lambda (or really any callable) that is called repeated for
    /// each allocated Base*
    template<typename F>
    void iterate(F f) {
      char * p = buffer;
      while (p < buffer+allocated) {
        Base* bp = reinterpret_cast<Base*>(p);
        p += bp->size();
        f(bp);
      }
    }
    
    template<typename OtherBase>
    friend void* ::operator new(size_t, Grappa::PoolAllocator<OtherBase>&);
  };

  template<size_t Bytes, typename Base>
  class PoolAllocatorInternal: public PoolAllocator<Base> {
    char _buffer[Bytes];
  public:
    PoolAllocatorInternal(): PoolAllocator<Base>(_buffer, Bytes) {}
  };
  
} // namespace Grappa


template<typename Base>
void* operator new(size_t size, Grappa::PoolAllocator<Base>& a) {
  return a.allocate(size);
}
