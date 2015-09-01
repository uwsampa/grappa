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

