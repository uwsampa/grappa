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
