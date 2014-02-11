////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
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



