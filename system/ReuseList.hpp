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

