
#ifndef __CONDITION_VARIABLE_HPP__
#define __CONDITION_VARIABLE_HPP__

#include "TaskingScheduler.hpp"
#include "Synchronization.hpp"
#include "Mutex.hpp"

namespace Grappa {

  /// @addtogroup Synchronization
  /// @{
  
  // Bare ConditionVariable. Use when you use a separate mutex or you
  // don't need an associated mutex.
  class ConditionVariable {
  public:
    intptr_t waiters_;

    ConditionVariable()
      : waiters_( 0 )
    { }
  };

  /// Verify that ConditionVariable is only one word
  static_assert( sizeof( ConditionVariable ) == 8, "ConditionVariable is not 64 bits for some reason.");

    
  // Hybrid Mutex/ConditionVariable. Use when your mutex and condition
  // variable can share a wait list.
  class MutexConditionVariable {
  public:
    union {
      struct {
	bool lock_ : 1;
	intptr_t waiters_ : 63;
      };
      intptr_t raw_; // unnecessary; just to ensure alignment
    };

    MutexConditionVariable()
      : lock_( false )
      , waiters_( 0 )
    { }
  };
    
  /// Verify that MutexConditionVariable is only one word
  static_assert( sizeof( MutexConditionVariable ) == 8, "MutexConditionVariable is not 64 bits for some reason.");

  /// Wait on a condition variable with a mutex. Mutex is released
  /// before the calling thread suspends, and acquired before the
  /// thread continues.
  template< typename ConditionVariable, typename Mutex >
  inline void wait( ConditionVariable * cv, Mutex * m ) {
    Worker * current = global_scheduler.get_current_thread();
    current->next = Grappa::impl::get_waiters( cv );
    Grappa::impl::set_waiters( cv, current );
    Grappa::unlock( m );
    global_scheduler.thread_suspend();
    Grappa::lock( m );
  }
    
  /// Wait on a condition variable (no mutex).
  template< typename ConditionVariable >
  inline void wait( ConditionVariable * cv ) {
    Worker * current = global_scheduler.get_current_thread();
    current->next = Grappa::impl::get_waiters( cv );
    Grappa::impl::set_waiters( cv, current );
    global_scheduler.thread_suspend();
  }

  /// Wake one waiter on a condition variable.
  template< typename ConditionVariable >
  inline void signal( ConditionVariable * cv ) {
    Worker * to_wake = Grappa::impl::get_waiters( cv );
    if( to_wake != NULL ) {
      Grappa::impl::set_waiters( cv, to_wake->next );
      global_scheduler.thread_wake( to_wake );
    }
  }
    
  /// Wake all waiters on a condition variable.
  template< typename ConditionVariable >
  inline void broadcast( ConditionVariable * cv ) {
    Worker * to_wake = NULL;
    while( ( to_wake = Grappa::impl::get_waiters( cv ) ) != NULL ) {
      Grappa::impl::set_waiters( cv, to_wake->next );
      global_scheduler.thread_wake( to_wake );
    }
  }
  
  /// Proxy for remote ConditionVariable manipulation
  /// @TODO: implement
  inline void wait( GlobalAddress<ConditionVariable> m ) {
    // if local, just wait
    // if remote, spawn a task on the home node to wait
  }

  /// TODO: implement
  inline void signal( GlobalAddress<ConditionVariable> m ) {
    // if local, just signal
    // if remote, signal via active message
  }

  /// TODO: implement
  inline void signal_all( GlobalAddress<ConditionVariable> m ) {
    // if local, just signal
    // if remote, signal via active message
  }

  /// @}

}

#endif
