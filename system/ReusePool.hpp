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
    DVLOG(5) << __PRETTY_FUNCTION__ << "/" << this << ": finished blocking until pop with " << s_.get_value() << " now";
    return ptrs_[s_.get_value()];
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
