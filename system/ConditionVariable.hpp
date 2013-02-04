
#ifndef __CONDITION_VARIABLE_HPP__
#define __CONDITION_VARIABLE_HPP__

#include "Message.hpp"
#include "Communicator.hpp"
#include "Addressing.hpp"
#include "ConditionVariableLocal.hpp"

namespace Grappa {

  /// @addtogroup Synchronization
  /// @{
  
  /// Proxy for remote ConditionVariable manipulation
  /// @TODO: implement
  inline void wait( GlobalAddress<ConditionVariable> m ) {
    // if local, just wait
    // if remote, spawn a task on the home node to wait
  }

  /// TODO: implement
  template<typename ConditionVariable>
  inline void signal( const GlobalAddress<ConditionVariable> m ) {
    if (m.node() == Grappa::mycore()) {
      // if local, just signal
      Grappa::signal(m.pointer());
    } else {
      // if remote, signal via active message
      auto _ = Grappa::send_message(m.node(), [=]{
        Grappa::signal(m.pointer());
      });
    }
  }

  /// TODO: implement
  inline void signal_all( GlobalAddress<ConditionVariable> m ) {
    // if local, just signal
    // if remote, signal via active message
  }

  /// @}

}

#endif
