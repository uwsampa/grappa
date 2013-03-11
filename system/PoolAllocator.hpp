#pragma once

#include <glog/logging.h>

namespace Grappa {

  template<typename Base>
  class PoolAllocator {
  protected:
    char * buffer;
    size_t buffer_size;
    size_t allocated;
    bool owns_buffer;
    
    Base* allocate(size_t sz) {
      DVLOG(4) << "allocating " << sz;
      Base* p = reinterpret_cast<Base*>(buffer+allocated);
      allocated += sz;
      
      // TODO: implement heap allocation fallback
      CHECK(allocated <= buffer_size) << "PoolAllocator overflow: tried allocating " << sz << ", already allocated " << allocated << " of " << buffer_size;
      return p;
    }
    
  public:
    PoolAllocator(char * buffer, size_t buffer_size, bool owns_buffer): buffer(buffer), buffer_size(buffer_size), allocated(0), owns_buffer(owns_buffer) {}
    
    void reset() {
      allocated = 0;
    }
    
    virtual ~PoolAllocator() {
      // call destructors of everything in PoolAllocator
      iterate([](Base* bp){ bp->~Base(); });
      if (owns_buffer) {
        delete [] buffer;
      }
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
    
    size_t remaining() { return buffer_size - allocated; }
    
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
