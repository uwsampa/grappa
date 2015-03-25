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
#pragma once

#include <limits>

#include "TaskingScheduler.hpp"
#include "Synchronization.hpp"

namespace Grappa {
  using namespace impl;
  
  /// @addtogroup Synchronization
  /// @{
  
  /// Counting semaphore. Maximum count is 2^15 - 1.
  class CountingSemaphore {
  private:
    inline void check( int64_t new_count ) const {
      DLOG_ASSERT( ( -(1<<15) <= new_count ) &&
                   ( new_count < (1<<15) ) ) << "adjusting semaphore by too much";
    }

  public:
    union {
      struct {
        int16_t count_ : 16;
        intptr_t waiters_ : 48;
      };
      intptr_t raw_; // unnecessary; just to ensure alignment
    };

    static const size_t max_value = (1 << 15) - 1;

    CountingSemaphore()
      : count_( 0 )
      , waiters_( 0 )
    { 
      DVLOG(5) << this << "/" << Grappa::impl::global_scheduler.get_current_thread() << ": Created with count=" << count_;
    }

    CountingSemaphore( int64_t initial_count )
      : count_( initial_count )
      , waiters_( 0 )
    { 
      check( initial_count );
      DVLOG(5) << this << "/" << Grappa::impl::global_scheduler.get_current_thread() << ": Created with count=" << count_;
    }

    // Decrement semaphore. Blocks if decrement would leave semaphore < 0.
    void reset( int64_t val = 0 ) {
      Grappa::broadcast( this );
      count_ = val;
    }

    // Increment semaphore. Wakes waiters
    inline void increment( int64_t incr = 1 ) {
      DVLOG(5) << this << "/" << Grappa::impl::global_scheduler.get_current_thread() << ": Incrementing " << count_ << " by " << incr;
      check( count_ + incr );  // verify this is possible
      count_ += incr; // 
      // TODO: is this a good choice?
      // we need to do something intelligent to support decrements != 1
      // the simplest correct thing is to wake all waiters,
      // but this may not be efficient.
      Grappa::broadcast( this );
    }
    
    // Decrement semaphore. Blocks if decrement would leave semaphore < 0.
    inline void decrement( int64_t decr = 1 ) {
      DVLOG(5) << this << "/" << Grappa::impl::global_scheduler.get_current_thread() << ": Ready to decrement " << count_ << " by " << decr;
      while( count_ - decr < 0 ) {
        DVLOG(5) << this << "/" << Grappa::impl::global_scheduler.get_current_thread() << ": Blocking to decrement " << count_ << " by " << decr;
        Grappa::wait( this );
        DVLOG(5) << this << "/" << Grappa::impl::global_scheduler.get_current_thread() << ": Woken to decrement " << count_ << " by " << decr;
      }
      check( count_ - decr ); // ensure this adjustment is possible....
      count_ -= decr;
    }

    // Try to decrement semaphore without blocking. Returns true if it
    // succeeds; false if it would block.
    bool try_decrement( int64_t decr = 1 ) {
      DVLOG(5) << this << "/" << Grappa::impl::global_scheduler.get_current_thread() << ": Trying to decrement " << count_ << " by " << decr;
      if( count_ - decr < 0 ) {
        DVLOG(5) << this << "/" << Grappa::impl::global_scheduler.get_current_thread() << ": Couldn't decrement " << count_ << " by " << decr;
        return false;
      }
      check( count_ - decr ); // ensure this adjustment is possible....
      count_ -= decr;
      return true;
    }
    
    inline int16_t get_value() const {
      return count_;
    }

  };
    
  /// Verify that CountingSemaphore is only one word
  static_assert( sizeof( CountingSemaphore ) == 8, "CountingSemaphore is not 64 bits for some reason.");

  /// @}

}

