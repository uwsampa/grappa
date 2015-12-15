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

/// Tests for delegate operations

#include <boost/test/unit_test.hpp>
#include <cstdio>

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

    std::remove("write.bin");
    writeTuplesUnordered<MaterializedTupleRef_V1_0_1>( &more_data, write_file );

    // try read
    Relation<MaterializedTupleRef_V1_0_1> results =
      readTuplesUnordered<MaterializedTupleRef_V1_0_1>( write_file );

    BOOST_CHECK_EQUAL( 2, results.numtuples );
    MaterializedTupleRef_V1_0_1 expected;
    if (one.get(0) == (*results.data.localize()).get(0)) {
      BOOST_CHECK_EQUAL( one.get(0), (*results.data.localize()).get(0) );
      BOOST_CHECK_EQUAL( one.get(1), (*results.data.localize()).get(1) );
    } else {
      BOOST_CHECK_EQUAL( two.get(0), (*results.data.localize()).get(0) );
      BOOST_CHECK_EQUAL( two.get(1), (*results.data.localize()).get(1) );
    }

    // write more
    MaterializedTupleRef_V1_0_1 three;
    MaterializedTupleRef_V1_0_1 four;
    three.set(0, 14);
    three.set(1, 15);
    four.set(0, 16);
    four.set(1, 17);
    more_data.clear();
    more_data.push_back(one);
    more_data.push_back(two);
    more_data.push_back(three);
    more_data.push_back(four);

    std::remove("write.bin");
    writeTuplesUnordered<MaterializedTupleRef_V1_0_1>( &more_data, write_file );

    // verify write
    results =
      readTuplesUnordered<MaterializedTupleRef_V1_0_1>( write_file );

    BOOST_CHECK_EQUAL( 4, results.numtuples );

    if ( one.get(0) == (*results.data.localize()).get(0) ) {
      BOOST_CHECK_EQUAL( one.get(0), (*results.data.localize()).get(0) );
      BOOST_CHECK_EQUAL( one.get(1), (*results.data.localize()).get(1) );
    } else if ( two.get(0) == (*results.data.localize()).get(0) ) {
      BOOST_CHECK_EQUAL( two.get(0), (*results.data.localize()).get(0) );
      BOOST_CHECK_EQUAL( two.get(1), (*results.data.localize()).get(1) );
    } else if ( three.get(0) == (*results.data.localize()).get(0) ) {
      BOOST_CHECK_EQUAL( three.get(0), (*results.data.localize()).get(0) );
      BOOST_CHECK_EQUAL( three.get(1), (*results.data.localize()).get(1) );
    } else {
      BOOST_CHECK_EQUAL( four.get(0), (*results.data.localize()).get(0) );
      BOOST_CHECK_EQUAL( four.get(1), (*results.data.localize()).get(1) );
    }
      

  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
