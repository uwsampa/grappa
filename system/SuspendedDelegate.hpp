#pragma once

#include <TaskingScheduler.hpp>
#include <functional>

namespace Grappa {

class SuspendedDelegate : public Worker {
  
  char func_storage[64];
  std::function<void()> saved_func;
  
public:
  SuspendedDelegate() {
    this->stack = nullptr;
    this->next = nullptr;
  }
  
  template< typename F >
  void setup(F func) {
    static_assert(sizeof(func) < 64, "func too large to store in suspended_delegate");
    
    new (func_storage) F(func);
    F* my_func = reinterpret_cast<F*>(func_storage);
    new (&saved_func) std::function<void()>([my_func]{ (*my_func)(); });
  }
  
  template< typename F > friend SuspendedDelegate* new_suspended_delegate(F);
  friend void invoke(SuspendedDelegate*);
};

static SuspendedDelegate * cont_freelist;

template< typename F >
SuspendedDelegate* new_suspended_delegate(F func) {
  SuspendedDelegate* c;
  if (cont_freelist != nullptr) {
    c = cont_freelist;
    cont_freelist = reinterpret_cast<SuspendedDelegate*>(c->next);
    new (c) SuspendedDelegate();
  } else {
    c = new SuspendedDelegate();
  }
  
  c->setup(func);
  return c;
}

inline void invoke(SuspendedDelegate * c) {
  // invoke
  c->saved_func();
  
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
