
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef __TIMESTAMP_HPP__
#define __TIMESTAMP_HPP__

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "common.hpp"

/// Increasing the value of this parameter may
/// reduce scheduler overhead.
DECLARE_uint64( timestamp_tick_freq );

namespace Grappa {

  /// Timestamps are 64-bit signed integers. Theoretically this should
  /// allow us to do overflow detection, but we ignore it for now. This
  /// will still allow many years of operation.
  typedef int64_t Timestamp;

  namespace impl {
    
    extern Grappa::Timestamp current_timestamp;
    
  }

  /// Grab a snapshot of the current value of the timestamp counter.
  static inline Grappa::Timestamp tick() {
    Grappa::Timestamp old_timestamp = impl::current_timestamp; 
    static int64_t count = FLAGS_timestamp_tick_freq;
    if( count-- <= 0 ) {
      impl::current_timestamp = rdtsc();
      count = FLAGS_timestamp_tick_freq;
    }
    return old_timestamp;
  }

  /// Grab a snapshot of the current value of the timestamp counter.
  static inline Grappa::Timestamp force_tick() {
    impl::current_timestamp = rdtsc();
    return impl::current_timestamp;
  }

  /// Return the current snapshot of the timestamp counter.
  static inline Grappa::Timestamp timestamp() { 
    //Grappa::tick();
    return impl::current_timestamp;
  }

}

#endif
