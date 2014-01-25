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
#include "GlobalMemoryChunk.hpp"
#include "GlobalAllocator.hpp"
#include "ParallelLoop.hpp"

BOOST_AUTO_TEST_SUITE( GlobalAllocator_tests );

const size_t local_size_bytes = 1 << 14;

struct Foo {
  long x, y;
} GRAPPA_BLOCK_ALIGNED;

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS, local_size_bytes );
  Grappa::run([]{
    GlobalAddress< int8_t > a = Grappa::global_alloc( 1 );
    LOG(INFO) << "got pointer " << a.pointer();

    GlobalAddress< int8_t > b = Grappa::global_alloc( 1 );
    LOG(INFO) << "got pointer " << b.pointer();

    GlobalAddress< int8_t > c = Grappa::global_alloc( 8 );
    LOG(INFO) << "got pointer " << c.pointer();

    GlobalAddress< int8_t > d = Grappa::global_alloc( 1 );
    LOG(INFO) << "got pointer " << d.pointer();

    BOOST_CHECK_EQUAL( global_allocator->total_bytes(), local_size_bytes );
    BOOST_CHECK_EQUAL( global_allocator->total_bytes_in_use(), 1 + 1 + 8 + 1 );

    LOG(INFO) << "freeing pointer " << c.pointer();
    Grappa::global_free( c );

    LOG(INFO) << "freeing pointer " << a.pointer();
    Grappa::global_free( a );

    LOG(INFO) << "freeing pointer " << d.pointer();
    Grappa::global_free( d );

    LOG(INFO) << "freeing pointer " << b.pointer();
    Grappa::global_free( b );

    BOOST_CHECK_EQUAL( global_allocator->total_bytes_in_use(), 0 );
    
    BOOST_MESSAGE("symmetric tests");
    
    auto s = Grappa::symmetric_global_alloc<Foo>();
    
    call_on_all_cores([s]{
      s->x = 1;
      s->y = 3;
    });
    
    LOG(INFO) << "done!";
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
