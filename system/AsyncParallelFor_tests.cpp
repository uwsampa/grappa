
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/test/unit_test.hpp>
#include "AsyncParallelFor.hpp"

BOOST_AUTO_TEST_SUITE( AsyncParallelFor_tests );

#define size 12
bool done[size] = {false};

void loop_body(int64_t start, int64_t num ) {
  BOOST_MESSAGE( "execute [" << start << ","
            << start+num << ")" );
  for (int i=start; i<start+num; i++) {
    BOOST_CHECK( !done[i] );
    done[i] = true;
  }
}

void spawn( void (*fp)(int64_t,int64_t), int64_t start, int64_t num ) {
  BOOST_MESSAGE ( "spawning [" << start << ","
            << start+num << ")" );
  fp ( start, num );
  BOOST_MESSAGE ( "completed [" << start << ","
            << start+num << ")" );

}

BOOST_AUTO_TEST_CASE( test1 ) {
  // parse command line flags
  google::ParseCommandLineFlags( &(boost::unit_test::framework::master_test_suite().argc),
                                 &(boost::unit_test::framework::master_test_suite().argv),
                                 true );
  
  async_parallel_for<&loop_body,&spawn> (0, size);
  for (int i=0; i<size; i++) {
    BOOST_CHECK( done[i] );
  }
}

BOOST_AUTO_TEST_SUITE_END();
