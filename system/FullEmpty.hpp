

// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
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
      //Grappa::signal( this );
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
      //Grappa::signal( this );
      return tt;
    }

  };
  /// @}

  template< typename T >
  void fill_remote(GlobalAddress<FullEmpty<T>> result_addr, const T& val) {
    send_heap_message(result_addr.core(), [result_addr,val]{
      result_addr->writeXF(val);
    });
  }
  
  template< typename T >
  T readFF(GlobalAddress<FullEmpty<T>> fe_addr) {
    if (fe_addr.core() == mycore()) return fe_addr.pointer()->readFE();
    
    auto worker_addr = make_global(impl::global_scheduler.get_current_thread());
    
    FullEmpty<T> result;
    auto result_addr = make_global(&result);
    
    send_message(fe_addr.core(), [fe_addr,result_addr]{
      auto& fe = *fe_addr.pointer();
      
      if (fe.full()) { fill_remote(result_addr, fe.readFE()); return; }
      
      auto* c = heap_continuation([fe_addr,result_addr]{
        VLOG(0) << "continuation!";
        fill_remote(result_addr, fe_addr->readFF());
      });
      add_waiter(&fe, c);
    });
    
    return result.readFF();
  }
  
}

#endif
