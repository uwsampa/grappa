
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

/// Timestamps are 64-bit signed integers. Theoretically this should
/// allow us to do overflow detection, but we ignore it for now. This
/// will still allow many years of operation.
typedef int64_t Grappa_Timestamp;
extern Grappa_Timestamp Grappa_current_timestamp;

/// Grab a snapshot of the current value of the timestamp counter.
static inline Grappa_Timestamp Grappa_tick() {
  Grappa_Timestamp old_timestamp = Grappa_current_timestamp; 
  static int64_t count = FLAGS_timestamp_tick_freq;
  if( count-- > 0 ) {
    Grappa_current_timestamp;
  } else {
    Grappa_current_timestamp = rdtsc();
    count = FLAGS_timestamp_tick_freq;
  }
  return old_timestamp;
}

static inline Grappa_Timestamp Grappa_force_tick() {
  Grappa_Timestamp old_timestamp = Grappa_current_timestamp; 
  Grappa_current_timestamp = rdtsc();
  return old_timestamp;
}
  
/// Return the current snapshot of the timestamp counter.
static inline Grappa_Timestamp Grappa_get_timestamp() { 
  //Grappa_tick();
  return Grappa_current_timestamp;
}

#endif
