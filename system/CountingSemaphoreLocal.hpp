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

    // Increment semaphore. Wakes waiters
    void increment( int64_t incr = 1 ) {
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
    void decrement( int64_t decr = 1 ) {
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

