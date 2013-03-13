// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.


#include <boost/test/unit_test.hpp>
#include "Grappa.hpp"
#include "Communicator.hpp"
#include "Collective.hpp"

// Tests the functions in Collective.hpp

BOOST_AUTO_TEST_SUITE( Collective_tests );

static int global_x;


void user_main( int * ignore ) {
  BOOST_MESSAGE("testing allreduce");
  Grappa::on_all_cores([]{
    int x = 7;
    int total_x = Grappa::allreduce<int,collective_add>(x);
    BOOST_CHECK_EQUAL(total_x, 7*Grappa::cores());
    
    global_x = Grappa::mycore() + 1;
  });

  BOOST_MESSAGE("testing reduce");
  int total_x = Grappa::reduce<int,collective_add>(&global_x);
  Core n = Grappa::cores();
  BOOST_CHECK_EQUAL(total_x, n*(n+1)/2);
  
  BOOST_MESSAGE("testing allreduce_inplace");
  Grappa::on_all_cores([]{
    int xs[10];
    for (int i=0; i<10; i++) xs[i] = i;
    
    Grappa::allreduce_inplace<int,collective_add>(xs, 10);
    
    for (int i=0; i<10; i++) BOOST_CHECK_EQUAL(xs[i], Grappa::cores() * i);
  });
}

BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  Grappa_activate();

  Grappa_run_user_main( &user_main, (int*)NULL );
  BOOST_CHECK( Grappa_done() == true );

  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

