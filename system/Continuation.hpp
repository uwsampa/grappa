#pragma once

#include <TaskingScheduler.hpp>
#include <functional>

namespace Grappa {

class Continuation : public Worker {
public:
  Continuation() {
    this->stack = nullptr;
    this->next = nullptr;
  }
  
  template< typename F >
  void setup(F func) {
    this->base = new std::function<void()>(func);
  }
  
  void invoke() {
    auto c = reinterpret_cast<std::function<void()>*>(this->base);
    (*c)();
    delete c;
    delete this;
  }
};

template< typename F >
Continuation* heap_continuation(F func) {
  auto* c = new Continuation();
  c->setup(func);
  return c;
}

inline bool is_continuation(Worker * w) { return w->stack == nullptr; }

} // namespace Grappa
