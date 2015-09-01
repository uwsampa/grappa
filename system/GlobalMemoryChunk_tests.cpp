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
