
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
#include "Tasking.hpp"

BOOST_AUTO_TEST_SUITE( FullEmpty_tests );

using namespace Grappa;

void test_remote_fe(int trial) {
  bool perturb = trial % 2 == 0;
  
  FullEmpty<long> _fe;
  auto fe = make_global(&_fe);
  
  CompletionEvent _ce(3);
  auto ce = make_global(&_ce);
  
  CompletionEvent ce1(1);
  privateTask([fe,ce,&ce1,trial]{
    ce1.complete();
    auto val = readFF(fe);
    BOOST_CHECK_EQUAL(val, 42);
    complete(ce);
    DVLOG(2) << "task<" << trial << ",1> done";
  });
   if (perturb) ce1.wait();
  
  send_message(1, [fe,ce,trial]{
    privateTask([fe,ce,trial]{
      privateTask([fe,ce,trial]{
        writeXF(fe, 42);
        complete(ce);
        DVLOG(2) << "task<" << trial << ",3> done";
      });
      
      long val = readFF(fe);
      BOOST_CHECK_EQUAL(val, 42);
      complete(ce);
      DVLOG(2) << "task<" << trial << ",2> done";
    });
  });
  // wait for other task to begin and start doing readFF
  // fe->writeEF(42);
  ce->wait();
}

void user_main( void * args ) 
{
  CompletionEvent ce;
  for (int i=0; i<100; i++) {
    test_remote_fe(i);
  }
  ce.wait();
  
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

