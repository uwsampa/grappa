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

#include <sys/mman.h>

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "Delegate.hpp"
#include "Cache.hpp"
#include "GlobalMemory.hpp"

BOOST_AUTO_TEST_SUITE( GlobalMemory_tests );

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS, (1L << 20) );
  Grappa::run([]{
    size_t N = 128;
    GlobalAddress<int64_t> data = Grappa::global_alloc<int64_t>(N);
  
    for (size_t i=0; i<N; i++) {
      BOOST_MESSAGE( "Writing " << i << " to " << data+i );
      Grappa::delegate::write(data+i, i);
    }
  
    for (size_t i=0; i<N; i++) {
      Incoherent<int64_t>::RO c(data+i, 1);
      VLOG(2) << i << " == " << *c;
      BOOST_CHECK_EQUAL(i, *c);
    }

    Grappa::global_free(data);
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
