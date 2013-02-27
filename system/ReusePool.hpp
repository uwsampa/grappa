#pragma once

// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

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
    , s_()
  { }

  bool available() {
    return s_.get_value() > 0;
  }

  int64_t count() {
    return s_.get_value();
  }

  T * block_until_pop() {
    s_.decrement();
    return ptrs_[s_.get_value()];
  }

  T * try_pop() {
    if( s_.try_decrement() ) {
      return ptrs_[ s_.get_value() ];
    } else {
      return NULL;
    }
  }

  void push( T * buf ) {
    CHECK_LT( s_.get_value(), max_count ) << "Can't check in buffer; maximum is " << max_count;
    ptrs_[ s_.get_value() ] = buf;
    s_.increment();
  }

  bool try_push( T * buf ) {
    if( s_.get_value() < max_count ) {
      push(buf);
      return true;
    } else {
      return false;
    }
  }
};


}
}
