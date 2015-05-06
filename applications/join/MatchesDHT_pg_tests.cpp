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

/// This file contains tests for the GlobalAddress<> class.

#include <boost/test/unit_test.hpp>

#include <Grappa.hpp>
#include "MatchesDHT_pg.hpp"
#include "Delegate.hpp"

BOOST_AUTO_TEST_SUITE( MatchesDHT_pg_tests );

using namespace Grappa;

typedef MatchesDHT<int64_t, int64_t, std::hash<int64_t>> DHT;
DHT table;

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([] {

    table.init_global_DHT(&table, cores() * 16 * 1024);

    table.insert_put_get(0, 100);

    auto size_and_ptr = table.lookup_size(0);
    auto size = size_and_ptr.get<0>();
    BOOST_CHECK_EQUAL(size, 1);

    auto ptr = size_and_ptr.get<1>();
    auto val = table.lookup_put_get(ptr, 0);
    BOOST_CHECK_EQUAL(val, 100);
  });

  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
