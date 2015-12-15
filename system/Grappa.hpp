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

#ifndef __GRAPPA_HPP__
#define __GRAPPA_HPP__

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "Communicator.hpp"
#include "Worker.hpp"
#include "tasks/TaskingScheduler.hpp"
#include "PerformanceTools.hpp"

///
/// Worker management routines
///

#include "Addressing.hpp"
#include "Tasking.hpp"
#include "StateTimer.hpp"

#include "Message.hpp"

#include "Delegate.hpp"
#include "AsyncDelegate.hpp"
#include "Collective.hpp"
#include "ParallelLoop.hpp"
#include "GlobalAllocator.hpp"
// #include "Cache.hpp"
#include "Array.hpp"

#include "Metrics.hpp"

//#include <typeinfo>
//#include <cxxabi.h>

/// Helper to be passed to Grappa::init() for Boost Unit Tests
#define GRAPPA_TEST_ARGS &(boost::unit_test::framework::master_test_suite().argc), &(boost::unit_test::framework::master_test_suite().argv) 

namespace Grappa {
  
  /// Initialize Grappa. Call in SPMD context before running Grappa
  /// code. Running Grappa code before calling init() is illegal.
  void init( int * argc_p, char ** argv_p[], int64_t size = -1 );

  /// Clean up Grappa. Call in SPMD context after all Grappa code
  /// finishes. Running Grappa code after calling finalize() is illegal.
  int finalize();
  
  
#ifndef GRAPPA_NO_ABBREV
  /// Specify non-default behavior: stealable tasks
  ///
  /// @code
  ///   spawn<unbound>(...)
  ///   forall<unbound>(...)
  ///   forall<unbound,async>(...)
  /// @endcode
  const auto unbound = Grappa::TaskMode::Unbound;
#endif
  
#ifndef GRAPPA_NO_ABBREV
  /// Specify non-blocking operation (to be used in loops, delegates, etc)
  /// 
  /// @code
  ///   forall<async>(...)
  ///   delegate::call<async>(...)
  ///   delegate::write<async>(...)
  /// @endcode
  const auto async = Grappa::SyncMode::Async;
#endif
  
}

#endif
