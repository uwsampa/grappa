#pragma once

// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "FullEmptyLocal.hpp"
#include "ConditionVariableLocal.hpp"


namespace Grappa {
  
/// @addtogroup Communication
/// @{
  
namespace impl {

/// List of buffers for RDMA messaging. Type must support get_next()
/// and set_next() calls.
template< typename T >
class ReuseList {
private:
  T * list_;
  ConditionVariable cv;
  size_t count_;

public:
  ReuseList()
    : list_(NULL)
    , cv()
    , count_(0)
  { }

  bool empty() const { 
    return list_ == NULL;
  }

  size_t count() const {
    return count_;
  }

  // block until we have a buffer to process
  T * block_until_pop() {
    while( empty() ) {
      DVLOG(5) << __PRETTY_FUNCTION__ << ": blocking";
      Grappa::wait( &cv );
    }

    T * b = list_;

    // put back rest of list
    list_ = b->get_next();

    DVLOG(5) << __PRETTY_FUNCTION__ << ": popping " << b << " (count " << count_ << ") list=" << list_;

    // make sure the buffer is not part of any list
    b->set_next( NULL );

    // record the get
    count_--;
    
    return b;
  }
  
  /// Returns NULL if no buffers are available, or a buffer otherwise.
  T * try_pop() {
    if( !empty() ) {
      return block_until_pop();
    } else {
      return NULL;
    }
  }
  
  void push( T * b ) {
    CHECK_NOTNULL( b );

    DVLOG(5) << __PRETTY_FUNCTION__ << ": pushing " << b << " (count " << count_ << ") list=" << list_;

    b->set_next( list_ );
    list_ = b;
    Grappa::signal( &cv );

    // record the put
    count_++;
  }

};

}

/// @}

}

