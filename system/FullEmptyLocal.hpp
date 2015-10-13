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

#ifndef __FULLEMPTY_HPP__
#define __FULLEMPTY_HPP__

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "ConditionVariableLocal.hpp"

namespace Grappa {
  
  /// @addtogroup Synchronization
  /// @{
  
  /// Wrapper class to provide full-bit semantics for arbitrary data.
  ///
  /// Each FullEmpty object holds a data element of type T, as well as
  /// a state variable. The state may be "empty," indicating that the
  /// data is invalid, or "full," indicating that the data is
  /// valid. 
  ///
  /// Data is accessed with methods that specify the desired state
  /// before and after the operation, and operations block (suspend)
  /// until the FullEmpty has the desired precursor state. It is also
  /// possible to specify that no precursor state is required.
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
    
    //private:
    T t_;
    
    void block_until( State desired_state ) {
      while( state_ != desired_state ) {
        DVLOG(5) << "In " << __PRETTY_FUNCTION__ 
                 << ", blocking until " << (desired_state == State::FULL ? "full" : "empty");
      	Grappa::wait( this );
      }
    }
    
  public:
    
    /// Construct a FullEmpty with an initial value, setting the state to empty.
    FullEmpty( ) : state_( State::EMPTY ), waiters_( 0 ), t_() {}

    /// Construct a FullEmpty with an initial value, setting the state to full.
    FullEmpty( T t ) : state_( State::FULL ), waiters_( 0 ), t_(t) {}

    /// returns true if the contents of the FullEmpty are valid.
    inline bool full() const { return state_ == State::FULL; }
    /// returns true if the contents of the FullEmpty are not valid.
    inline bool empty() const { return state_ == State::EMPTY; }
    
    /// Clears the state of the full bit to empty, marking the contents as invalid.
    inline void reset() {
      CHECK(waiters_ == 0);
      state_ = State::EMPTY;
    }
    
  /// Write data to FullEmpty no matter what its current state, leaving it full. Any existing contents are overwritten.
    T writeXF( T t ) {
      t_ = t; 
      state_ = State::FULL; 
      Grappa::broadcast( this );
      //Grappa::signal( this );
      return t_; 
    }
      
    /// Suspend until FullEmpty state is empty, and then store the argument in the contents and set the state to full.
    T writeEF( T t ) { 
      block_until(State::EMPTY); 
      T tt = writeXF( t );
      return tt;
    }

    /// Suspend until FullEmpty state is full, and then overwrites the current contents, leaving the state set as full.
    T writeFF( T t ) { 
      block_until(State::FULL); 
      T tt = writeXF( t );
      return tt;
    }


    /// (DANGER: For debugging only.)
    ///
    /// Return contents of FullEmpty no matter what its current state,
    /// leaving the state unchanged. This may return garbage.
    T readXX() {
      return t_; 
    }

    /// Suspend until FullEmpty is full, and then return its contents, leaving it full.      
    ///
    /// @returns 
    T readFF() { 
      block_until(State::FULL); 
      T tt = readXX();
      Grappa::broadcast( this );
      //Grappa::signal( this );
      return tt;
    }

    /// Suspend until FullEmpty is full, and then return its contents, leaving it empty.      
    T readFE() {
      block_until(State::FULL);
      state_ = State::EMPTY;
      T tt = readXX();
      Grappa::broadcast( this );
      //Grappa::signal( this );
      return tt;
    }

  };

  /// Non-member version of writeXF method. @see FullEmpty::writeXF
  template< typename T >
  T writeXF( FullEmpty<T> * fe_addr, T t ) {
    return fe_addr->writeXF(t);
  }

  /// Non-member version of writeEF method. @see FullEmpty::writeEF
  template< typename T >
  T writeEF( FullEmpty<T> * fe_addr, T t ) {
    return fe_addr->writeEF(t);
  }

  /// Non-member version of writeFF method. @see FullEmpty::writeFF
  template< typename T >
  T writeFF( FullEmpty<T> * fe_addr, T t ) {
    return fe_addr->writeFF(t);
  }

  /// Non-member version of readXX method. @see FullEmpty::readXX
  template< typename T >
  T readXX( FullEmpty<T> * fe_addr) {
    return fe_addr->readXX();
  }

  /// Non-member version of readFF method. @see FullEmpty::readFE
  template< typename T >
  T readFF( FullEmpty<T> * fe_addr) {
    return fe_addr->readFF();
  }

  /// Non-member version of readFE method. @see FullEmpty::readFE
  template< typename T >
  T readFE( FullEmpty<T> * fe_addr) {
    return fe_addr->readFE();
  }

  /// @}
  
}

#endif
