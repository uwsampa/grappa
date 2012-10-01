
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <sys/mman.h>

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "Delegate.hpp"
#include "GlobalMemoryChunk.hpp"
#include "GlobalAllocator.hpp"

BOOST_AUTO_TEST_SUITE( GlobalAllocator_tests );


const size_t local_size_bytes = 1 << 10;

void test( Thread * me, void* args ) {
  GlobalAddress< int64_t > a = Grappa_malloc( 1 );
  LOG(INFO) << "got pointer " << a.pointer();

  GlobalAddress< int64_t > b = Grappa_typed_malloc< int64_t >( 1 );
  LOG(INFO) << "got pointer " << b.pointer();

  GlobalAddress< int64_t > c = Grappa_malloc( 8 );
  LOG(INFO) << "got pointer " << c.pointer();

  GlobalAddress< int64_t > d = Grappa_malloc( 1 );
  LOG(INFO) << "got pointer " << d.pointer();

  LOG(INFO) << *global_allocator;

  if( Grappa_mynode() == 0 ) {
    BOOST_CHECK_EQUAL( global_allocator->total_bytes(), local_size_bytes * Grappa_nodes() );
    BOOST_CHECK_EQUAL( global_allocator->total_bytes_in_use(), 1 + 8 + 8 + 1 );
  }

  LOG(INFO) << "freeing pointer " << c.pointer();
  Grappa_free( c );

  LOG(INFO) << "freeing pointer " << a.pointer();
  Grappa_free( a );

  LOG(INFO) << "freeing pointer " << d.pointer();
  Grappa_free( d );

  LOG(INFO) << "freeing pointer " << b.pointer();
  Grappa_free( b );

  if( Grappa_mynode() == 0 ) {
    BOOST_CHECK_EQUAL( global_allocator->total_bytes_in_use(), 0 );
  }
  LOG(INFO) << *global_allocator;
  
  GlobalAddress<int64_t> * vargs = reinterpret_cast< GlobalAddress<int64_t> * >( args );
  //Grappa_flush( vargs->node() );
  Grappa_delegate_fetch_and_add_word( *vargs, 1 );
  Grappa_flush( vargs->node() );
}

void user_main( Thread * me, void * args ) 
{
  int count = 0;
  GlobalAddress< int64_t > global_count = make_global( &count );
  //Grappa_spawn( &test, &global_count );
  //test( NULL, &global_count );
  //test( NULL, &global_count );
  Grappa_spawn( &test, &global_count );
  //Grappa_spawn( &test, &global_count );
  //Grappa_flush( 0 );

  Grappa_remote_spawn( &test, &global_count, 1 );

  while( count < 2 ) {
    //LOG(INFO) << "count is " << 2;
    Grappa_yield();
  }

  BOOST_CHECK_EQUAL( global_allocator->total_bytes_in_use(), 0 );
  LOG(INFO) << *global_allocator;

  Grappa_signal_done();
}

BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv),
                local_size_bytes * 2);

  assert( Grappa_nodes() == 2 );
  Grappa_activate();

  DVLOG(1) << "Spawning user main Thread....";
  Grappa_run_user_main( &user_main, NULL );
  BOOST_CHECK( Grappa_done() == true );

  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

