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
#include <sys/mman.h>

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "Delegate.hpp"

DECLARE_int64( global_memory_per_node_base_address );

namespace Grappa { namespace impl { extern void * global_memory_chunk_base; } }

BOOST_AUTO_TEST_SUITE( GlobalMemoryChunk_tests );

GlobalAddress< int64_t > base;
size_t size;

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  
  size_t local_size_bytes = 1 << 8;
  // cheat. we know this 
  base = make_linear( static_cast<int64_t*>(Grappa::impl::global_memory_chunk_base) );
  
  BOOST_MESSAGE( "Base pointer is " << base );

  size = local_size_bytes * Grappa::cores() / sizeof(int64_t);

  BOOST_CHECK_EQUAL( Grappa::cores(), 2 );

  DVLOG(1) << "Spawning user main Worker....";
  
  Grappa::run([]{
    for( int i = 0; i < size; ++i ) {
      BOOST_MESSAGE( "Writing to " << base + i );
      Grappa::delegate::write( base + i, i );
    }

    for( int i = 0; i < size; ++i ) {
      int64_t remote_data = Grappa::delegate::read( base + i );
      BOOST_CHECK_EQUAL( remote_data, i );
    }

    // this all assumes a 64-byte block cyclic distribution
    BOOST_CHECK_EQUAL( (base +  0).pointer(), base.pointer() +  0 );
    BOOST_CHECK_EQUAL( (base +  1).pointer(), base.pointer() +  1 );
    BOOST_CHECK_EQUAL( (base +  2).pointer(), base.pointer() +  2 );
    BOOST_CHECK_EQUAL( (base +  3).pointer(), base.pointer() +  3 );
    BOOST_CHECK_EQUAL( (base +  4).pointer(), base.pointer() +  4 );
    BOOST_CHECK_EQUAL( (base +  5).pointer(), base.pointer() +  5 );
    BOOST_CHECK_EQUAL( (base +  6).pointer(), base.pointer() +  6 );
    BOOST_CHECK_EQUAL( (base +  7).pointer(), base.pointer() +  7 );
    BOOST_CHECK_EQUAL( (base +  8).pointer(), base.pointer() +  0 );
    BOOST_CHECK_EQUAL( (base +  9).pointer(), base.pointer() +  1 );
    BOOST_CHECK_EQUAL( (base + 10).pointer(), base.pointer() +  2 );
    BOOST_CHECK_EQUAL( (base + 11).pointer(), base.pointer() +  3 );
    BOOST_CHECK_EQUAL( (base + 12).pointer(), base.pointer() +  4 );
    BOOST_CHECK_EQUAL( (base + 13).pointer(), base.pointer() +  5 );
    BOOST_CHECK_EQUAL( (base + 14).pointer(), base.pointer() +  6 );
    BOOST_CHECK_EQUAL( (base + 15).pointer(), base.pointer() +  7 );
    BOOST_CHECK_EQUAL( (base + 16).pointer(), base.pointer() +  8 );
    BOOST_CHECK_EQUAL( (base + 17).pointer(), base.pointer() +  9 );
    BOOST_CHECK_EQUAL( (base + 18).pointer(), base.pointer() + 10 );
    BOOST_CHECK_EQUAL( (base + 19).pointer(), base.pointer() + 11 );
    BOOST_CHECK_EQUAL( (base + 20).pointer(), base.pointer() + 12 );
    BOOST_CHECK_EQUAL( (base + 21).pointer(), base.pointer() + 13 );
    BOOST_CHECK_EQUAL( (base + 22).pointer(), base.pointer() + 14 );
    BOOST_CHECK_EQUAL( (base + 23).pointer(), base.pointer() + 15 );
    BOOST_CHECK_EQUAL( (base + 24).pointer(), base.pointer() +  8 );
    BOOST_CHECK_EQUAL( (base + 25).pointer(), base.pointer() +  9 );
    BOOST_CHECK_EQUAL( (base + 26).pointer(), base.pointer() + 10 );
    BOOST_CHECK_EQUAL( (base + 27).pointer(), base.pointer() + 11 );
    BOOST_CHECK_EQUAL( (base + 28).pointer(), base.pointer() + 12 );
    BOOST_CHECK_EQUAL( (base + 29).pointer(), base.pointer() + 13 );
    BOOST_CHECK_EQUAL( (base + 30).pointer(), base.pointer() + 14 );
    BOOST_CHECK_EQUAL( (base + 31).pointer(), base.pointer() + 15 );

  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
