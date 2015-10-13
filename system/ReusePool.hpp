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

#include <vector>
#include "Semaphore.hpp"


namespace Grappa {
namespace impl {
    

template< typename T,
          typename Semaphore = CountingSemaphore, 
          int max_count = CountingSemaphore::max_value >
class ReusePool {
private:
  CountingSemaphore s_;
  T * ptrs_[ max_count ];

public:
  ReusePool() 
    : ptrs_()
    , s_(0)
  { }

  bool available() {
    return s_.get_value() > 0;
  }

  int64_t count() {
    return s_.get_value();
  }

  T * block_until_pop() {
    DVLOG(5) << __PRETTY_FUNCTION__ << "/" << this << ": blocking until pop with " << s_.get_value() << " now";
    s_.decrement();
    T * result = ptrs_[s_.get_value()];
    DVLOG(5) << __PRETTY_FUNCTION__ << "/" << this << ": finished blocking until pop with " << s_.get_value() << "/" << result;
    return result;
  }

  T * try_pop() {
    DVLOG(5) << __PRETTY_FUNCTION__ << "/" << this << ": trying to pop with " << s_.get_value() << " now";
    if( s_.try_decrement() ) {
      T * t = ptrs_[ s_.get_value() ]; 
      DVLOG(5) << __PRETTY_FUNCTION__ << "/" << this << ": succeeded; popping " << t << " with " << s_.get_value() << " now";
      return t;
    } else {
      return NULL;
    }
  }

  void push( T * buf ) {
    DVLOG(5) << __PRETTY_FUNCTION__ << "/" << this << ": pushing " << buf << " with " << s_.get_value() << " already";
    CHECK_LT( s_.get_value(), max_count ) << "Can't check in buffer; maximum is " << max_count;
    ptrs_[ s_.get_value() ] = buf;
    s_.increment();
  }

  bool try_push( T * buf ) {
    DVLOG(5) << __PRETTY_FUNCTION__ << "/" << this << ": trying to push " << buf << " with " << s_.get_value() << " already";
    if( s_.get_value() < max_count ) {
      push(buf);
      DVLOG(5) << __PRETTY_FUNCTION__ << "/" << this << ": succeeded; pushed " << buf << " with " << s_.get_value() << " now";
      return true;
    } else {
      return false;
    }
  }
};


}
}
