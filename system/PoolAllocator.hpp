////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
////////////////////////////////////////////////////////////////////////

#pragma once

#include <glog/logging.h>
#include "LocaleSharedMemory.hpp"

namespace Grappa {
  /// @addtogroup Utility
  /// @{

  template<typename Base>
  class PoolAllocator {
  public:
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
        Grappa::impl::locale_shared_memory.deallocate(buffer);
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
  
  /// @}
} // namespace Grappa


template<typename Base>
void* operator new(size_t size, Grappa::PoolAllocator<Base>& a) {
  return a.allocate(size);
}
