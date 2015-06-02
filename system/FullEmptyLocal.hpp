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
// Energy. The Government has certain rights in the software.


#ifndef __FULLEMPTY_HPP__
#define __FULLEMPTY_HPP__

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "ConditionVariableLocal.hpp"

namespace Grappa {
  
  /// @addtogroup Synchronization
  /// @{
  
  /// Wrapper class to provide full-bit semantics for arbitrary data.
  template< typename T >
  class FullEmpty {
  private:
    enum class State : bool { EMPTY = false, FULL = true };

  public:
    union {
      struct {
        State state_ : 1;
        intptr_t waiters_ : 63; // can't include pointers in bitfield, so use intptr_t
      };
      intptr_t raw_; // unnecessary; just to ensure alignment
    };
    
  private:
    T t_;
    
    void block_until( State desired_state ) {
      while( state_ != desired_state ) {
        DVLOG(5) << "In " << __PRETTY_FUNCTION__ 
                 << ", blocking until " << (desired_state == State::FULL ? "full" : "empty");
      	Grappa::wait( this );
      }
    }
    
  public:
    
    FullEmpty( ) : state_( State::EMPTY ), waiters_( 0 ), t_() {}

    FullEmpty( T t ) : state_( State::FULL ), waiters_( 0 ), t_(t) {}

    inline bool full() const { return state_ == State::FULL; }
    inline bool empty() const { return state_ == State::EMPTY; }
    
    inline void reset() {
      CHECK(waiters_ == 0);
      state_ = State::EMPTY;
    }
    
    T writeXF( T t ) {
      t_ = t; 
      state_ = State::FULL; 
      Grappa::broadcast( this );
      //Grappa::signal( this );  // signaling only to avoid unsafe broadcast
      return t_; 
    }
      
    T writeEF( T t ) { 
      block_until(State::EMPTY); 
      T tt = writeXF( t );
      return tt;
    }

    T writeFF( T t ) { 
      block_until(State::FULL); 
      T tt = writeXF( t );
      return tt;
    }


    T readXX() {
      return t_; 
    }
      
    T readFF() { 
      block_until(State::FULL); 
      T tt = readXX();
      Grappa::broadcast( this );
      //Grappa::signal( this );
      return tt;
    }

    T readFE() {
      block_until(State::FULL);
      state_ = State::EMPTY;
      T tt = readXX();
      Grappa::broadcast( this );
      //Grappa::signal( this );   // signal safe only if guarentee waiters require Empty, otherwise broadcast
      return tt;
    }

  };
  /// @}
  
}

#endif
