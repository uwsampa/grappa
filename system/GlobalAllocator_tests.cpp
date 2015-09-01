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

#include <sys/mman.h>

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "Delegate.hpp"
#include "GlobalMemoryChunk.hpp"
#include "GlobalAllocator.hpp"
#include "ParallelLoop.hpp"

BOOST_AUTO_TEST_SUITE( GlobalAllocator_tests );

const size_t local_size_bytes = 1 << 14;

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
  
    LOG(INFO) << "done!";
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
