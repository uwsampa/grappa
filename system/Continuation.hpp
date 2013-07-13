#pragma once

#include <TaskingScheduler.hpp>
#include <functional>

namespace Grappa {

class Continuation : public Worker {
  
  char func_storage[64];
  std::function<void()> saved_func;
  
public:
  Continuation() {
    this->stack = nullptr;
    this->next = nullptr;
  }
  
  template< typename F >
  void setup(F func) {
    static_assert(sizeof(func) < 64, "func too large to store in continuation");
    
    new (func_storage) F(func);
    F* my_func = reinterpret_cast<F*>(func_storage);
    new (&saved_func) std::function<void()>([my_func]{ (*my_func)(); });
  }
  
  template< typename F > friend Continuation* new_continuation(F);
  friend void invoke(Continuation*);
};

static Continuation * cont_freelist;

template< typename F >
Continuation* new_continuation(F func) {
  Continuation* c;
  if (cont_freelist != nullptr) {
    c = cont_freelist;
    cont_freelist = reinterpret_cast<Continuation*>(c->next);
    new (c) Continuation();
  } else {
    c = new Continuation();
  }
  
  c->setup(func);
  return c;
}

inline void invoke(Continuation * c) {
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

inline bool is_continuation(Worker * w) { return w->stack == nullptr; }

} // namespace Grappa
