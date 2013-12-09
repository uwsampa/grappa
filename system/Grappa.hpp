
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef __GRAPPA_HPP__
#define __GRAPPA_HPP__

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "Communicator.hpp"
#include "Worker.hpp"
#include "tasks/TaskingScheduler.hpp"
#include "PerformanceTools.hpp"

//#include <typeinfo>
//#include <cxxabi.h>

/// Helper to be passed to Grappa::init() for Boost Unit Tests
#define GRAPPA_TEST_ARGS &(boost::unit_test::framework::master_test_suite().argc), &(boost::unit_test::framework::master_test_suite().argv) 

namespace Grappa {

  extern double tick_rate;

  namespace impl {

    /// global scheduler instance
    extern TaskingScheduler global_scheduler;

    /// called on failures to backtrace and pause for debugger
    extern void failure_function();
    
    void signal_done();
    
  }

  /// Initialize Grappa. Call in SPMD context before running Grappa
  /// code. Running Grappa code before calling init() is illegal.
  void init( int * argc_p, char ** argv_p[], size_t size = -1 );

  /// Clean up Grappa. Call in SPMD context after all Grappa code
  /// finishes. Running Grappa code after calling finalize() is illegal.
  int finalize();


  /// pointer to parent pthread
  extern Worker * master_thread;

}

/// for convenience/brevity, define macro to get current thread/worker
/// pointer
#define CURRENT_THREAD Grappa::impl::global_scheduler.get_current_thread()

extern Core * node_neighbors;

void Grappa_init( int * argc_p, char ** argv_p[], size_t size = -1 );

void Grappa_activate();

bool Grappa_done();
int Grappa_finish( int retval );


///
/// Worker management routines
///

#include "Addressing.hpp"
#include "Tasking.hpp"
#include "StateTimer.hpp"

#endif
