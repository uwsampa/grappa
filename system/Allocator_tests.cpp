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

/// Tests for generic buddy allocator.

#include "Allocator.hpp"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE( Allocator_tests );

BOOST_AUTO_TEST_CASE( init ) {
  char foo[ 1024 ];
  Allocator a( &foo[0], 1024 );

  BOOST_CHECK_EQUAL( a.num_chunks(), 1 );
  BOOST_CHECK_EQUAL( a.total_bytes(), 1024 );
  BOOST_CHECK_EQUAL( a.total_bytes_in_use(), 0 );
  BOOST_CHECK_EQUAL( a.total_bytes_free(), 1024 );
  BOOST_MESSAGE( a );
}

BOOST_AUTO_TEST_CASE( non_power_2 ) {
  char foo[ 1024 + 64 ];
  Allocator a( &foo[0], 1024 + 64 );

  BOOST_CHECK_EQUAL( a.num_chunks(), 2 );
  BOOST_CHECK_EQUAL( a.total_bytes(), 1024 + 64 );
  BOOST_CHECK_EQUAL( a.total_bytes_in_use(), 0 );
  BOOST_CHECK_EQUAL( a.total_bytes_free(), 1024 + 64 );
  BOOST_MESSAGE( a );
}

BOOST_AUTO_TEST_CASE( non_power_22 ) {
  char foo[ 1234 ];
  Allocator a( &foo[0], 1234 );

  BOOST_CHECK_EQUAL( a.num_chunks(), 5 );
  BOOST_CHECK_EQUAL( a.total_bytes(), 1234 );
  BOOST_CHECK_EQUAL( a.total_bytes_in_use(), 0 );
  BOOST_CHECK_EQUAL( a.total_bytes_free(), 1234 );
  BOOST_MESSAGE( a );
}

BOOST_AUTO_TEST_CASE( allocate ) {
  char foo[ 1024 + 64 + 32 ];
  Allocator a( &foo[0], 1024 + 64 + 32 );

  BOOST_CHECK_EQUAL( a.num_chunks(), 3 );
  BOOST_CHECK_EQUAL( a.total_bytes(), 1024 + 64 + 32 );
  BOOST_CHECK_EQUAL( a.total_bytes_in_use(), 0 );
  BOOST_CHECK_EQUAL( a.total_bytes_free(), 1024 + 64 + 32 );
  BOOST_MESSAGE( a );

  void * bar = a.malloc( 64 );
  BOOST_CHECK_EQUAL( a.num_chunks(), 3 );
  BOOST_CHECK_EQUAL( a.total_bytes(), 1024 + 64 + 32 );
  BOOST_CHECK_EQUAL( a.total_bytes_in_use(), 64 );
  BOOST_CHECK_EQUAL( a.total_bytes_free(), 1024 + 32 );
  BOOST_MESSAGE( "after malloc: " << a );

  a.free( bar );
  BOOST_CHECK_EQUAL( a.num_chunks(), 3 );
  BOOST_CHECK_EQUAL( a.total_bytes(), 1024 + 64 + 32 );
  BOOST_CHECK_EQUAL( a.total_bytes_in_use(), 0 );
  BOOST_CHECK_EQUAL( a.total_bytes_free(), 1024 + 64 + 32 );  
  BOOST_MESSAGE( "after free: " << a );

  void * baz = a.malloc( 72 );
  BOOST_CHECK_EQUAL( a.num_chunks(), 6 );
  BOOST_CHECK_EQUAL( a.total_bytes(), 1024 + 64 + 32 );
  BOOST_CHECK_EQUAL( a.total_bytes_in_use(), 128 );
  BOOST_CHECK_EQUAL( a.total_bytes_free(), 1024 - 128 + 64 + 32 );
  BOOST_MESSAGE( "after malloc of baz: " << a );

  void * splat = a.malloc( 72 );
  BOOST_CHECK_EQUAL( a.num_chunks(), 6 );
  BOOST_CHECK_EQUAL( a.total_bytes(), 1024 + 64 + 32 );
  BOOST_CHECK_EQUAL( a.total_bytes_in_use(), 256 );
  BOOST_CHECK_EQUAL( a.total_bytes_free(), 1024 - 128 - 128 + 64 + 32 );
  BOOST_MESSAGE( "after malloc of splat: " << a );

  void * blah = a.malloc( 72 );
  BOOST_CHECK_EQUAL( a.num_chunks(), 7 );
  BOOST_CHECK_EQUAL( a.total_bytes(), 1024 + 64 + 32 );
  BOOST_CHECK_EQUAL( a.total_bytes_in_use(), 384 );
  BOOST_CHECK_EQUAL( a.total_bytes_free(), 1024 - 128 - 128 - 128 + 64 + 32 );
  BOOST_MESSAGE( "after malloc of blah: " << a );

  a.free( splat );
  BOOST_CHECK_EQUAL( a.num_chunks(), 7 );
  BOOST_CHECK_EQUAL( a.total_bytes(), 1024 + 64 + 32 );
  BOOST_CHECK_EQUAL( a.total_bytes_in_use(), 256 );
  BOOST_CHECK_EQUAL( a.total_bytes_free(), 1024 - 128 - 128 + 64 + 32 );  
  BOOST_MESSAGE( "after free of splat: " << a );

  a.free( baz );
  BOOST_CHECK_EQUAL( a.num_chunks(), 6 );
  BOOST_CHECK_EQUAL( a.total_bytes(), 1024 + 64 + 32 );
  BOOST_CHECK_EQUAL( a.total_bytes_in_use(), 128 );
  BOOST_CHECK_EQUAL( a.total_bytes_free(), 1024 - 128 + 64 + 32 );  
  BOOST_MESSAGE( "after free of baz: " << a );

  a.free( blah );
  BOOST_CHECK_EQUAL( a.num_chunks(), 3 );
  BOOST_CHECK_EQUAL( a.total_bytes(), 1024 + 64 + 32 );
  BOOST_CHECK_EQUAL( a.total_bytes_in_use(), 0 );
  BOOST_CHECK_EQUAL( a.total_bytes_free(), 1024 + 64 + 32 );  
  BOOST_MESSAGE( "after free of blah: " << a );
}

BOOST_AUTO_TEST_CASE( small ) {
  char foo[ 1024 ];
  Allocator a( &foo[0], 1024 );

  BOOST_CHECK_EQUAL( a.num_chunks(), 1 );
  BOOST_CHECK_EQUAL( a.total_bytes(), 1024 );
  BOOST_CHECK_EQUAL( a.total_bytes_in_use(), 0 );
  BOOST_CHECK_EQUAL( a.total_bytes_free(), 1024 );
  BOOST_MESSAGE( a );
  
  void * ptrs[ 128 ];

  for( int i = 0; i < 128; ++i ) {
    ptrs[ i ] = a.malloc( 7 );
  }

  BOOST_CHECK_EQUAL( a.num_chunks(), 128 );
  BOOST_CHECK_EQUAL( a.total_bytes(), 1024 );
  BOOST_CHECK_EQUAL( a.total_bytes_in_use(), 1024 );
  BOOST_CHECK_EQUAL( a.total_bytes_free(), 0 );
  BOOST_MESSAGE( "after malloc: " << a );

  for( int i = 0; i < 128; ++i ) {
    a.free( ptrs[ i ] );
  }
  BOOST_CHECK_EQUAL( a.num_chunks(), 1 );
  BOOST_CHECK_EQUAL( a.total_bytes(), 1024 );
  BOOST_CHECK_EQUAL( a.total_bytes_in_use(), 0 );
  BOOST_CHECK_EQUAL( a.total_bytes_free(), 1024 );  
  BOOST_MESSAGE( "after free: " << a );
}

BOOST_AUTO_TEST_CASE( toobig ) {
  char foo[ 1024 ];
  Allocator a( &foo[0], 1024 );

  BOOST_CHECK_THROW( a.malloc( 1024 + 64 ), Allocator::Exception );
}

BOOST_AUTO_TEST_SUITE_END();
