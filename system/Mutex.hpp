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

#ifndef __MUTEX_HPP__
#define __MUTEX_HPP__

#include "Communicator.hpp"
#include "TaskingScheduler.hpp"
#include "Synchronization.hpp"
#include "Addressing.hpp"
#include "ConditionVariableLocal.hpp"

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
      Grappa::wait( t );
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

  template< typename Mutex >
  inline bool is_unlocked( Mutex * t ) { return t->lock_ == false; }

  /// Unlock a mutex. Note: wait scheme is unfairly LIFO
  template< typename Mutex >
  inline void unlock( Mutex * t ) {
    Grappa::impl::compiler_memory_fence();
    // release lock unconditionally
    t->lock_ = false;
    Grappa::signal(t);
  }

  /// TODO: implement
  template< typename Mutex >
  inline void lock( GlobalAddress<Mutex> m ) {
    // if local, just acquire
    if( m.core() == mycore() ) {
      lock( m.pointer() );
    } else { // if remote, spawn a task on the home node to acquire 
      CHECK(false);
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



