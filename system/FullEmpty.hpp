

// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.


#ifndef __FULLEMPTY_HPP__
#define __FULLEMPTY_HPP__

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "ConditionVariable.hpp"

namespace Grappa {
  
  /// @addtogroup Synchronization
  /// @{
  
  /// Wrapper class to provide full-bit semantics for arbitrary data.
  template< typename T >
  class FullEmpty {
  private:
    enum class State : bool { EMPTY = false, FULL = true };

    union {
      struct {
	State state_ : 1;
	intptr_t waiters_ : 63; // can't include pointers in bitfield, so use intptr_t
      };
      intptr_t raw_; // unnecessary; just to ensure alignment
    };

    T t_;

    void block_until( State desired_state ) {
      while( state_ != desired_state ) {
	Grappa::wait( this );
      }
    }
    
  public:
    FullEmpty( ) : state_( State::EMPTY ), waiters_( 0 ), t_() {}

    inline bool full() { return state_ == State::FULL; }

    T writeXF( T t ) {
      t_ = t; 
      state_ = State::FULL; 
      Grappa::broadcast( this );
      return t_; 
    }
      
    T writeEF( T t ) { 
      block_until(State::EMPTY); 
      return writeXF( t );
    }

    T writeFF( T t ) { 
      block_until(State::FULL); 
      return writeXF( t );
    }


    T readXX() {
      return t_; 
    }
      
    T readFF() { 
      block_until(State::FULL); 
      return readXX();
    }

    T readFE() { 
      block_until(State::FULL); 
      state_ = State::EMPTY;
      Grappa::broadcast( this );
      return readXX();
    }

  };
  /// @}

}

#endif
