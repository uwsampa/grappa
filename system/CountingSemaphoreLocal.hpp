#pragma once

// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "TaskingScheduler.hpp"
#include "Synchronization.hpp"
#include "Mutex.hpp"

namespace Grappa {

  /// @addtogroup Synchronization
  /// @{
  
  // Counting semaphore. Maximum count is 2^15.
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

    CountingSemaphore()
      : count_( 0 )
      , waiters_( 0 )
    { 
      DVLOG(5) << this << "/" << global_scheduler.get_current_thread() << ": Created with count=" << count_;
    }

    CountingSemaphore( int64_t initial_count )
      : count_( initial_count )
      , waiters_( 0 )
    { 
      check( initial_count );
      DVLOG(5) << this << "/" << global_scheduler.get_current_thread() << ": Created with count=" << count_;
    }

    // Increment semaphore. Wakes waiters
    void increment( int64_t incr = 1 ) {
      DVLOG(5) << this << "/" << global_scheduler.get_current_thread() << ": Incrementing " << count_ << " by " << incr;
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
      DVLOG(5) << this << "/" << global_scheduler.get_current_thread() << ": Ready to decrement " << count_ << " by " << decr;
      while( count_ - decr < 0 ) {
        DVLOG(5) << this << "/" << global_scheduler.get_current_thread() << ": Blocking to decrement " << count_ << " by " << decr;
        Grappa::wait( this );
        DVLOG(5) << this << "/" << global_scheduler.get_current_thread() << ": Woken to decrement " << count_ << " by " << decr;
      }
      check( count_ - decr ); // ensure this adjustment is possible....
      count_ -= decr;
    }

    // Try to decrement semaphore without blocking. Returns true if it
    // succeeds; false if it would block.
    bool try_decrement( int64_t decr = 1 ) {
      DVLOG(5) << this << "/" << global_scheduler.get_current_thread() << ": Trying to decrement " << count_ << " by " << decr;
      if( count_ - decr < 0 ) {
        DVLOG(5) << this << "/" << global_scheduler.get_current_thread() << ": Couldn't decrement " << count_ << " by " << decr;
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
    
  /// Verify that MutexConditionVariable is only one word
  static_assert( sizeof( CountingSemaphore ) == 8, "CountingSemaphore is not 64 bits for some reason.");

  /// @}

}

