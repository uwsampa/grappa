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
  #define unbound Grappa::TaskMode::Unbound
#endif
  
#ifndef GRAPPA_NO_ABBREV
  /// Specify non-blocking operation (to be used in loops, delegates, etc)
  /// 
  /// @code
  ///   forall<async>(...)
  ///   delegate::call<async>(...)
  ///   delegate::write<async>(...)
  /// @endcode
  #define async Grappa::SyncMode::Async
#endif
  
}

#endif
