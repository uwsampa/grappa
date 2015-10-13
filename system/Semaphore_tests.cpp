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

