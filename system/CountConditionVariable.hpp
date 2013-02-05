#pragma once

#include "ConditionVariableLocal.hpp"

namespace Grappa {

  /// Reincarnation of 'LocalTaskJoiner' idea
  class CountConditionVariable: public ConditionVariable {
    int64_t count;
  public:
    CountConditionVariable(int64_t count = 0): count(count) {}
    
    void increment(int64_t inc = 1) {
      count += inc;
    }
    
    /// Decrement count once, if count == 0, wake all waiters.
    void signal() {
      if (count == 0) {
        LOG(ERROR) << "too many calls to signal()";
      }
      count--;
      if (count == 0) {
        broadcast(this);
      }
    }
    
    friend inline void signal(CountConditionVariable * cv);
  };

  /// Overload Grappa::signal() so it can be used like a ConditionVariable.
  inline void signal( CountConditionVariable * ccv ) {
    ccv->signal();
  }

} // namespace Grappa
