
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "FullEmpty.hpp"
#include "CompletionEvent.hpp"
#include "Collective.hpp"

BOOST_AUTO_TEST_SUITE( FullEmpty_tests );

using namespace Grappa;

void test_remote_fe() {
  FullEmpty<long> _fe;
  auto fe = make_global(&_fe);
  
  CompletionEvent _ce(2);
  auto ce = make_global(&_ce);
  
  privateTask([fe,ce]{
    auto val = readFF(fe);
    BOOST_CHECK_EQUAL(val, 42);
    complete(ce);
  });
  
  on_all_cores([fe,ce]{
    if (mycore() != 1) return;
    
    CompletionEvent lce(1);
    privateTask([fe,&lce,ce]{
      lce.complete();
      long val = readFF(fe);
      BOOST_CHECK_EQUAL(val, 42);
      complete(ce);
    });
    lce.wait();
  });
  // wait for other task to begin and start doing readFF
  fe->writeEF(42);
  ce->wait();
}

void user_main( void * args ) 
{
  test_remote_fe();
  
  Grappa::FullEmpty< int64_t > fe_int;

  BOOST_CHECK_EQUAL( fe_int.readXX(), 0 );

  fe_int.writeXF( 1 );

  BOOST_CHECK_EQUAL( fe_int.full(), true );
  BOOST_CHECK_EQUAL( fe_int.readXX(), 1 );

  BOOST_CHECK_EQUAL( fe_int.readFE(), 1 );
  fe_int.writeEF( 2 );
  BOOST_CHECK_EQUAL( fe_int.readFE(), 2 );
  fe_int.writeEF( 3 );
  BOOST_CHECK_EQUAL( fe_int.readFE(), 3 );
  fe_int.writeEF( 1 );

  return;

  Grappa::privateTask( [&] { 
      int64_t temp = 0;
      while( (temp = fe_int.readFE()) != 1 ) { 
	fe_int.writeEF( temp ); 
      }

      fe_int.writeEF( 2 );

      while( (temp = fe_int.readFE()) != 3 ) { 
	fe_int.writeEF( temp ); 
      }

      fe_int.writeEF( 4 );

    } );

  int64_t temp = 0;
  
  while( (temp = fe_int.readFE()) != 2 ) { 
    fe_int.writeEF( temp ); 
  }
  
  fe_int.writeEF( 3 );
  
  while( (temp = fe_int.readFE()) != 4 ) { 
    fe_int.writeEF( temp ); 
  }
  
  fe_int.writeEF( 5 );

  BOOST_CHECK_EQUAL( fe_int.readFE(), 5 );

  Grappa_merge_and_dump_stats();
}

BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
	       &(boost::unit_test::framework::master_test_suite().argv),
	       (1L << 20) );

  Grappa_activate();

  Grappa_run_user_main( &user_main, (void*)NULL );

  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

