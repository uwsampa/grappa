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
