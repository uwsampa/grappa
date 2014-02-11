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

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "LocaleSharedMemory.hpp"
#include "ParallelLoop.hpp"

BOOST_AUTO_TEST_SUITE( LocaleSharedMemory_tests );

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS, 1<<23 );
  Grappa::run([]{

    int64_t * arr = NULL;

    LOG(INFO) << "Allocating";
  
    // allocate array
    if( Grappa::locale_mycore() == 0 ) {
      // we need to do this on each node (not core)
      arr = static_cast< int64_t* >( Grappa::impl::locale_shared_memory.allocate_aligned( sizeof(int64_t) * Grappa::locale_cores(), 1<<3 ) );
      for( int i = 0; i < Grappa::locale_cores(); ++i ) {
        arr[i] = 0;
      }
    }

    LOG(INFO) << "Setting address";
    Grappa::on_all_cores( [arr] {
        int other_index = Grappa::locale_cores() - Grappa::locale_mycore() - 1;
        arr[ other_index ] = Grappa::locale_mycore();
      });

    LOG(INFO) << "Checking";
    Grappa::on_all_cores( [arr] {
        int other_index = Grappa::locale_cores() - Grappa::locale_mycore() - 1;
        BOOST_CHECK_EQUAL( arr[ Grappa::locale_mycore() ], other_index );
      });

    LOG(INFO) << "Done";
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();

