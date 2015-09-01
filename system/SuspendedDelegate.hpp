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

#include <TaskingScheduler.hpp>
#include <functional>

namespace Grappa {

  namespace impl {
    template< typename F >
    void func_proxy(F* f) { (*f)(); }
  }

class SuspendedDelegate : public Worker {
  
  void(*func_fn)(void*);
  
  char func_storage[64];
  
  // std::function<void()> saved_func;
  
public:
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
    
    func_fn = reinterpret_cast<void(*)(void*)>(impl::func_proxy<F>);
    
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

inline void invoke(SuspendedDelegate * c) {
  // invoke
  // c->saved_func();
  c->func_fn(c->func_storage);
  
  // add back into freelist
  c->next = nullptr;
  if (cont_freelist == nullptr) {
    cont_freelist = c;
  } else {
    c->next = reinterpret_cast<Worker*>(cont_freelist);
    cont_freelist = c;
  }
}

inline bool is_suspended_delegate(Worker * w) { return w->stack == nullptr; }

} // namespace Grappa
