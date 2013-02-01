
#ifndef __MUTEX_HPP__
#define __MUTEX_HPP__

#include "Communicator.hpp"
#include "TaskingScheduler.hpp"
#include "Synchronization.hpp"
#include "Addressing.hpp"

namespace Grappa {
  
  class Mutex {
  public:
    union {
      struct {
        bool lock_ : 1;
        intptr_t waiters_ : 63;
      };
      intptr_t raw_; // unnecessary; just to ensure alignment
    };
    Mutex()
      : lock_( false )
      , waiters_( 0 )
    { }
  };

  /// Verify that Mutex is only one word
  static_assert( sizeof( Mutex ) == 8, "Mutex is not 64 bits for some reason.");

  /// Lock a mutex. Note: wait scheme is unfairly LIFO
  template< typename Mutex >
  inline void lock( Mutex * t ) {
    while( true == t->lock_ ) { // while lock is held,
      // add us to queue
      Worker * current = global_scheduler.get_current_thread();
      current->next = Grappa::impl::get_waiters( t );
      Grappa::impl::set_waiters( t, current );
      /// and sleep
      global_scheduler.thread_suspend();
    }
    // lock is no longer held, so acquire
    t->lock_ = true;
    Grappa::impl::compiler_memory_fence();
  }

  /// Try to lock a mutex. Note: wait scheme is unfairly LIFO
  template< typename Mutex >
  inline bool trylock( Mutex * t ) {
    if( true == t->lock_ ) { // if lock is held,
      return false;
    }
    // lock not held, so acquire
    t->lock_ = true;
    Grappa::impl::compiler_memory_fence();
    return true;
  }

  /// Unlock a mutex. Note: wait scheme is unfairly LIFO
  template< typename Mutex >
  inline void unlock( Mutex * t ) {
    Grappa::impl::compiler_memory_fence();
    // release lock unconditionally
    t->lock_ = false;
    if( t->waiters_ ) { // if anyone was waiting
      // wake one up
      Grappa::Worker * to_wake = Grappa::impl::get_waiters( t );
      Grappa::impl::set_waiters( t, to_wake->next );
      to_wake->next = NULL;
      global_scheduler.thread_wake( to_wake );
    }
  }

  /// TODO: implement
  template< typename Mutex >
  inline void lock( GlobalAddress<Mutex> m ) {
    // if local, just acquire
    if( m.node() == global_communicator.mynode() ) {
      lock( m.pointer() );
    } else { // if remote, spawn a task on the home node to acquire 
    }
  }

  /// TODO: implement
  template< typename Mutex >
  inline void trylock( GlobalAddress<Mutex> m ) {

    // if local, just acquire
    // if remote, spawn a task on the home node to acquire 
  }

  /// TODO: implement
  template< typename Mutex >
  inline void unlock( GlobalAddress<Mutex> m ) {
    // if local, just acquire
    // if remote, spawn a task on the home node to acquire 
  }

  /// @}

}

#endif



