
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

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

BOOST_AUTO_TEST_SUITE_END();
