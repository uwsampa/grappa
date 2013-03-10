
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

//make -j TARGET=Yield_tests.test mpi_test VERBOSE_TESTS=1 GARGS="--yields=10000000 --threads=1 --periodic_poll_ticks=10000000 --stats_blob_ticks=3000000000" PPN=1 NNODE=1

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"

DEFINE_int64( threads, 2048, "#Threads");
DEFINE_int64( yields, 10000, "Total #yields");
bool not_done = 1;

BOOST_AUTO_TEST_SUITE( Yield_tests );

void child1( Thread * t, void * arg ) {
  while (not_done) Grappa_yield();
}

double calcInterval( struct timespec start, struct timespec end ) {
    const double BILLION_ = 1000000000.0;
    return ((end.tv_sec*BILLION_ + end.tv_nsec) - (start.tv_sec*BILLION_ + start.tv_nsec))/BILLION_;
}

void user_main( void * args ) 
{
  char * str = (char *) args;
  BOOST_MESSAGE( "Spawning user main thread " << (void *) CURRENT_THREAD <<
                 " " << CURRENT_THREAD <<
                 " " << str <<
                 " on node " << Grappa_mynode() );
  for (int i = 1; i < FLAGS_threads; i++) {
    Grappa_spawn(&child1, (void*) &i);
  }

  struct timespec start, end;
  clock_gettime( CLOCK_MONOTONIC, &start);

  int yields_per_thread = FLAGS_yields/FLAGS_threads;
  for (int i = 0; i < yields_per_thread; i++) {
    Grappa_yield();
  }
  not_done = 0;

  clock_gettime( CLOCK_MONOTONIC, &end);
  double runtime_s = calcInterval( start, end );
  double ns_per_yield = (runtime_s*1000000000) / FLAGS_yields;
  BOOST_MESSAGE( FLAGS_threads << " threads." );
  BOOST_MESSAGE( yields_per_thread << " yields per thread." );
  BOOST_MESSAGE( ns_per_yield << " ns per yield (avg)" );  
}

BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  Grappa_activate();

  BOOST_MESSAGE( "main before user main thread " << (void *) CURRENT_THREAD <<
                 " on node " << Grappa_mynode() );
  char str[] = "user main";
  Grappa_run_user_main( &user_main, (void*) str );
  BOOST_CHECK( Grappa_done() == true );
  BOOST_MESSAGE( "main after user main thread " << (void *) CURRENT_THREAD <<
                 " on node " << Grappa_mynode() );

  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

