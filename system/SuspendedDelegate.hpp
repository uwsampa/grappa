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
