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

/// Tests for delegate operations

#include <boost/test/unit_test.hpp>

#include <Grappa.hpp>
#include <GlobalAllocator.hpp>

#include "relation_io.hpp"

using namespace Grappa;

BOOST_AUTO_TEST_SUITE( Relation_io_tests );

class MaterializedTupleRef_V1_0_1 {
  public:
    int64_t _fields[2];

    int64_t get(int field) const {
      return _fields[field];
    }

    void set(int field, int64_t val) {
      _fields[field] = val;
    }

    int numFields() const {
      return 2;
    }

    MaterializedTupleRef_V1_0_1 () {
      // no-op
    }

    MaterializedTupleRef_V1_0_1 (std::vector<int64_t> vals) {
      for (int i=0; i<vals.size(); i++) _fields[i] = vals[i];
    }

    std::ostream& dump(std::ostream& o) const {
      o << "Materialized(";
      for (int i=0; i<numFields(); i++) {
        o << _fields[i] << ",";
      }
      o << ")";
      return o;
    }

  } GRAPPA_BLOCK_ALIGNED;
  std::ostream& operator<< (std::ostream& o, const MaterializedTupleRef_V1_0_1& t) {
    return t.dump(o);
  }

int64_t other_data __attribute__ ((aligned (2048))) = 0;
std::vector<MaterializedTupleRef_V1_0_1> more_data;

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{

    BOOST_CHECK_EQUAL( 2, Grappa::cores() );

    // write to new file
    std::string write_file = "write.bin";
    MaterializedTupleRef_V1_0_1 one;
    MaterializedTupleRef_V1_0_1 two;
    one.set(0, 10);
    one.set(1, 11);
    two.set(0, 12);
    two.set(1, 13);
    more_data.push_back(one);
    more_data.push_back(two);

    writeTuplesUnordered<MaterializedTupleRef_V1_0_1>( &more_data, write_file );

    // try read
    Relation<MaterializedTupleRef_V1_0_1> results =
      readTuplesUnordered<MaterializedTupleRef_V1_0_1>( write_file );

    BOOST_CHECK_EQUAL( 2, results.numtuples );
    MaterializedTupleRef_V1_0_1 expected;
    expected.set(0, 10);
    expected.set(1, 11);
    BOOST_CHECK_EQUAL( expected.get(0), (*results.data.localize()).get(0) );
    BOOST_CHECK_EQUAL( expected.get(1), (*results.data.localize()).get(1) );


    // write to existing file
    MaterializedTupleRef_V1_0_1 three;
    MaterializedTupleRef_V1_0_1 four;
    three.set(0, 14);
    three.set(1, 15);
    four.set(0, 16);
    four.set(1, 17);
    more_data.clear();
    more_data.push_back(three);
    more_data.push_back(four);

    writeTuplesUnordered<MaterializedTupleRef_V1_0_1>( &more_data, write_file );

    // verify write
    results =
      readTuplesUnordered<MaterializedTupleRef_V1_0_1>( write_file );

    BOOST_CHECK_EQUAL( 4, results.numtuples );

    expected.set(0, 10);
    expected.set(1, 11);
    BOOST_CHECK_EQUAL( expected.get(0), (*results.data.localize()).get(0) );
    BOOST_CHECK_EQUAL( expected.get(1), (*results.data.localize()).get(1) );

  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
