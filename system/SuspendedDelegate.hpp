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

#include <TaskingScheduler.hpp>
#include <functional>

namespace Grappa {

  namespace impl {
    template< typename F >
    bool func_proxy(F* f) { return (*f)(); }
  }

class SuspendedDelegate : public Worker {
public:
  using Callback = bool(void*);

  Callback* func_fn;
  
  char func_storage[64];
  
  // std::function<void()> saved_func;
  
  SuspendedDelegate() {
    this->stack = nullptr;
    this->next = nullptr;
  }
    
  template< typename F >
  static SuspendedDelegate* create(F f);
  
  template< typename F >
  void setup(F func) {
    static_assert(sizeof(func) < 64, "func too large to store in suspended_delegate");
    new (func_storage) F(func);
    
    func_fn = reinterpret_cast<Callback*>(impl::func_proxy<F>);
    
    // F* my_func = reinterpret_cast<F*>(func_storage);
    // saved_func = [my_func]{ (*my_func)(); };
    // saved_func = func;
  }
  
  template< typename F > friend SuspendedDelegate* new_suspended_delegate(F);
  friend void invoke(SuspendedDelegate*);
};

static SuspendedDelegate * cont_freelist;

template< typename F >
SuspendedDelegate* SuspendedDelegate::create(F f) {
  SuspendedDelegate* c;
  if (cont_freelist != nullptr) {
    c = cont_freelist;
    cont_freelist = reinterpret_cast<SuspendedDelegate*>(c->next);
    new (c) SuspendedDelegate();
  } else {
    c = new SuspendedDelegate();
  }
  c->setup(f);
  return c;
}

template< typename Sync >
inline void invoke(SuspendedDelegate * c, Sync* sync) {
  // invoke
  // c->saved_func();
  bool done = c->func_fn(c->func_storage);
  
  if (done) {
    // add back into freelist
    c->next = nullptr;
    if (cont_freelist == nullptr) {
      cont_freelist = c;
    } else {
      c->next = reinterpret_cast<Worker*>(cont_freelist);
      cont_freelist = c;
    }
  } else {
    // block again
    add_waiter(sync, c);
  }
}

inline bool is_suspended_delegate(Worker * w) { return w->stack == nullptr; }

} // namespace Grappa
