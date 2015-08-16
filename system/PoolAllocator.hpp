////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
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
