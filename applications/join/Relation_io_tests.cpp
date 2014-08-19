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

int64_t other_data __attribute__ ((aligned (2048))) = 0;

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

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{
    
    BOOST_CHECK_EQUAL( 2, Grappa::cores() );
    
    // try read
    Relation<MaterializedTupleRef_V1_0_1> results = 
      readTuplesUnordered<MaterializedTupleRef_V1_0_1>( "test.bin");

    BOOST_CHECK_EQUAL( 5, results.numtuples );


    //    MaterializedTupleRef_V1_0_1 expected;
    //    expected.set(0, 0);
    //    expected.set(1, 1);
    
    
    /*
    // write
    Grappa::delegate::write( make_global(&some_data,1), 2345 );
    BOOST_CHECK_EQUAL( 1111, some_data );
  
    // verify write
    remote_data = delegate::read( make_global(&some_data,1) );
    BOOST_CHECK_EQUAL( 2345, remote_data );

    // fetch and add
    remote_data = delegate::fetch_and_add( make_global(&some_data,1), 1 );
    BOOST_CHECK_EQUAL( 1111, some_data );
    BOOST_CHECK_EQUAL( 2345, remote_data );
  
    // verify write
    remote_data = delegate::read( make_global(&some_data,1) );
    BOOST_CHECK_EQUAL( 2346, remote_data );

    // check compare_and_swap
    bool swapped;
    swapped = delegate::compare_and_swap( make_global(&some_data,1), 123, 3333); // shouldn't swap
    BOOST_CHECK_EQUAL( swapped, false );
    // verify value is unchanged
    remote_data = delegate::read( make_global(&some_data,1) );
    BOOST_CHECK_EQUAL( 2346, remote_data );
  
    // now actually do swap
    swapped = delegate::compare_and_swap( make_global(&some_data,1), 2346, 3333);
    BOOST_CHECK_EQUAL( swapped, true );
    // verify value is changed
    remote_data = delegate::read( make_global(&some_data,1) );
    BOOST_CHECK_EQUAL( 3333, remote_data );
  
    // try linear global address
    
    // initialize
    auto i64_per_block = block_size / sizeof(int64_t);
    
    call_on_all_cores([]{ other_data = mycore(); });
    
    int * foop = new int;
    *foop = 1234;
    BOOST_MESSAGE( *foop );
    

    // hack the test
    // void* prev_base = Grappa::impl::global_memory_chunk_base;
    call_on_all_cores([]{
      Grappa::impl::global_memory_chunk_base = 0;
    });

    // make address
    BOOST_MESSAGE( "pointer is " << &other_data );

    GlobalAddress< int64_t > la = make_linear( &other_data );
    
    // check pointer computation
    BOOST_CHECK_EQUAL( la.core(), 0 );
    BOOST_CHECK_EQUAL( la.pointer(), &other_data );

    // check data
    BOOST_CHECK_EQUAL( 0, other_data );
    remote_data = delegate::read( la );
    BOOST_CHECK_EQUAL( 0, remote_data );
    
    // change pointer and check computation
    ++la;
    BOOST_CHECK_EQUAL( la.core(), 0 );
    BOOST_CHECK_EQUAL( la.pointer(), &other_data + 1 );

    // change pointer and check computation
    la += (i64_per_block-1);
    BOOST_CHECK_EQUAL( la.core(), 1 );
    
    // check remote data
    remote_data = delegate::read( la );
    BOOST_CHECK_EQUAL( 1, remote_data );

    // check template read
    // try read
    remote_data = delegate::read( make_global(&some_data,1) );
    BOOST_CHECK_EQUAL( 3333, remote_data );
		 */    
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
