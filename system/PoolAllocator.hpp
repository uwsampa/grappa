#pragma once

#include <glog/logging.h>

namespace Grappa {

  template<size_t Bytes, typename Base>
  class PoolAllocator {
    char buffer[Bytes];
    size_t allocated;
  protected:
    Base* allocate(size_t sz) {
      LOG(ERROR) << "allocating " << sz;
      Base* p = reinterpret_cast<Base*>(buffer+allocated);
      allocated += sz;
      
      // TODO: implement heap allocation fallback
      if (allocated > Bytes) { LOG(ERROR) << "PoolAllocator overflow!"; exit(1); }
      return p;
    }
    
  public:
    PoolAllocator(): allocated(0) {}
    
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
    
    template<size_t OtherBytes, typename OtherBase>
    friend void* ::operator new(size_t, Grappa::PoolAllocator<OtherBytes,OtherBase>&);
  };

} // namespace Grappa


template<size_t Bytes,typename Base>
void* operator new(size_t size, Grappa::PoolAllocator<Bytes,Base>& a) {
  return a.allocate(size);
}
