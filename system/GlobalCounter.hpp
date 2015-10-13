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

#include "Addressing.hpp"
#include "FlatCombiner.hpp"
#include "LocaleSharedMemory.hpp"
#include "GlobalAllocator.hpp"
#include "Collective.hpp"

namespace Grappa {

class GlobalCounter {
public:
  
  struct Master {
    long count;
    Core core;
  } master;
  
  GlobalAddress<GlobalCounter> self;
  
  struct Proxy {
    GlobalCounter* outer;
    long delta;
    
    Proxy(GlobalCounter* outer): outer(outer), delta(0) {}
    Proxy* clone_fresh() { return locale_new<Proxy>(outer); }
    
    void sync() {
      auto s = outer->self;
      auto d = delta;
      delegate::call(outer->master.core, [s,d]{ s->master.count += d; });
    }
    
    void clear() { delta = 0; }
    bool is_full() { return false; }
  };
  FlatCombiner<Proxy> comb;
    
  GlobalCounter(GlobalAddress<GlobalCounter> self, long initial_count = 0, Core master_core = 0): self(self), comb(locale_new<Proxy>(this)) {
    master.count = initial_count;
    master.core = master_core;
  }
  
  void incr(long d = 1) {
    comb.combine([d](Proxy& p){
      p.delta += d;
      return FCStatus::BLOCKED;
    });
  }
  
  long count() {
    auto s = self;
    return delegate::call(master.core, [s]{ return s->master.count; });
  }
  
  static GlobalAddress<GlobalCounter> create(long initial_count = 0) {
    auto a = symmetric_global_alloc<GlobalCounter>();
    auto master_core = mycore();
    call_on_all_cores([a,initial_count,master_core]{ new (a.localize()) GlobalCounter(a, initial_count, master_core); });
    return a;
  }
  
  void destroy() {
    auto a = self;
    call_on_all_cores([a]{ a->~GlobalCounter(); });
  }
} GRAPPA_BLOCK_ALIGNED;

} // namespace Grappa
