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
#include "Mutex.hpp"

BOOST_AUTO_TEST_SUITE( Mutex_tests );

using namespace Grappa;

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{
    Grappa::Mutex m;

    int data = 0;

    Grappa::lock( &m );
    data++;
    Grappa::unlock( &m );

    Worker * t = impl::global_scheduler.get_current_thread();

    spawn([&] { 
      lock( &m ); 
      data++; 
      unlock( &m ); 
      if( t ) {
      	impl::global_scheduler.thread_wake(t);
      }
    });

    impl::global_scheduler.thread_suspend();

    Grappa::lock( &m );
    BOOST_CHECK_EQUAL( data, 2 );
    Grappa::unlock( &m );

    // Grappa_merge_and_dump_stats();
  });
}

BOOST_AUTO_TEST_SUITE_END();

