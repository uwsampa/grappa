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
