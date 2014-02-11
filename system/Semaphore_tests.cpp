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

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "Semaphore.hpp"
#include "Metrics.hpp"
#include "TaskingScheduler.hpp"

BOOST_AUTO_TEST_SUITE( Semaphore_tests );

using namespace Grappa;

void yield() {
  Worker * thr = impl::global_scheduler.get_current_thread();
  spawn( [thr] {
      impl::global_scheduler.thread_wake( thr );
    });
  impl::global_scheduler.thread_suspend();
}


BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{
    CountingSemaphore s(1);

    // bit vector
    int data = 1;
    int count = 0;
    CompletionEvent ce(6);
    
    spawn([&s,&data,&count,&ce]{
      for( int i = 0; i < 3; ++i ) {
        decrement( &s );
        VLOG(1) << "Task 1 running.";
        data <<= 1;
        data |= 1;
        count++;
        ce.complete();
        increment( &s );
        yield();
      }
    });

    spawn([&s,&data,&count,&ce]{
      for( int i = 0; i < 3; ++i ) {
        decrement( &s );
        VLOG(1) << "Task 2 running.";
        data <<= 1;
        count++;
        ce.complete();
        increment( &s );
        yield();
      }
    });

    ce.wait();    
    

    BOOST_CHECK_EQUAL( count, 6 );
  
    Metrics::merge_and_dump_to_file();
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();

