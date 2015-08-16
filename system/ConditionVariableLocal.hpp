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

#ifndef __CONDITION_VARIABLE_LOCAL_HPP__
#define __CONDITION_VARIABLE_LOCAL_HPP__

#include "TaskingScheduler.hpp"
#include "SuspendedDelegate.hpp"
#include "Synchronization.hpp"
// #include "Mutex.hpp"

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
  // class MutexConditionVariable {
  // public:
  //   union {
  //     struct {
  //       bool lock_ : 1;
  //       intptr_t waiters_ : 63;
  //     };
  //     intptr_t raw_; // unnecessary; just to ensure alignment
  //   };
  // 
  //   MutexConditionVariable()
  //     : lock_( false )
  //     , waiters_( 0 )
  //   { }
  // };
  //   
  // /// Verify that MutexConditionVariable is only one word
  // static_assert( sizeof( MutexConditionVariable ) == 8, "MutexConditionVariable is not 64 bits for some reason.");
  // 
  // /// Wait on a condition variable with a mutex. Mutex is released
  // /// before the calling thread suspends, and acquired before the
  // /// thread continues.
  // template< typename ConditionVariable, typename Mutex >
  // inline void wait( ConditionVariable * cv, Mutex * m ) {
  //   Worker * current = impl::global_scheduler.get_current_thread();
  //   current->next = Grappa::impl::get_waiters( cv );
  //   Grappa::impl::set_waiters( cv, current );
  //   Grappa::unlock( m );
  //   impl::global_scheduler.thread_suspend();
  //   Grappa::lock( m );
  // }
    
  /// Wait on a condition variable (no mutex).
  template< typename ConditionVariable >
  inline void add_waiter( ConditionVariable * cv, Worker * w ) {
    w->next = Grappa::impl::get_waiters( cv );
    Grappa::impl::set_waiters( cv, w );
  }
  
  template< typename ConditionVariable >
  inline void wait( ConditionVariable * cv ) {
    Worker * current = impl::global_scheduler.get_current_thread();
    add_waiter(cv, current);
    impl::global_scheduler.thread_suspend();
  }

  /// Wake one waiter on a condition variable.
  template< typename ConditionVariable >
  inline void signal( ConditionVariable * cv ) {
    Worker * to_wake = Grappa::impl::get_waiters( cv );
    if( to_wake != NULL ) {
      Grappa::impl::set_waiters( cv, to_wake->next );
      to_wake->next = NULL;
      if (is_suspended_delegate(to_wake)) {
        invoke(reinterpret_cast<SuspendedDelegate*>(to_wake));
      } else {
        impl::global_scheduler.thread_wake( to_wake );
      }
    }
  }
    
  /// Wake all waiters on a condition variable.
  template< typename ConditionVariable >
  inline void broadcast( ConditionVariable * cv ) {
    Worker * to_wake = NULL;
    while( ( to_wake = Grappa::impl::get_waiters( cv ) ) != NULL ) {
      Grappa::impl::set_waiters( cv, to_wake->next );
      to_wake->next = NULL;
      if (is_suspended_delegate(to_wake)) {
        invoke(reinterpret_cast<SuspendedDelegate*>(to_wake));
      } else {
        impl::global_scheduler.thread_wake( to_wake );
      }
    }
  }
  
  /// @}

}

#endif
