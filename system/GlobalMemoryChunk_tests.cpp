
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <gflags/gflags.h>
#include <sys/mman.h>

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "Delegate.hpp"

DECLARE_int64( global_memory_per_node_base_address );

BOOST_AUTO_TEST_SUITE( GlobalMemoryChunk_tests );


GlobalAddress< int64_t > base;
size_t size;

void user_main( void * args ) 
{
  for( int i = 0; i < size; ++i ) {
    BOOST_MESSAGE( "Writing to " << base + i );
    Grappa_delegate_write_word( base + i, i );
  }

  for( int i = 0; i < size; ++i ) {
    int64_t remote_data = Grappa_delegate_read_word( base + i );
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


}

BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  Grappa_activate();

  size_t local_size_bytes = 1 << 8;
  // cheat. we know this 
  base = make_linear( (void*) FLAGS_global_memory_per_node_base_address );
  BOOST_MESSAGE( "Base pointer is " << base );

  size = local_size_bytes * Grappa_nodes() / sizeof(int64_t);

  BOOST_CHECK_EQUAL( Grappa_nodes(), 2 );

  DVLOG(1) << "Spawning user main Thread....";
  Grappa_run_user_main( &user_main, (void*) NULL );
  BOOST_CHECK( Grappa_done() == true );

  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

