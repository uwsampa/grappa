// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/test/unit_test.hpp>

#include "SoftXMT.hpp"
#include "Uid.hpp"
#include "Collective.hpp"
#include "common.hpp"
#include "ForkJoin.hpp"
#include "AsyncParallelFor.hpp"
#include "GlobalTaskJoiner.hpp"
#include "Delegate.hpp"
#include "DictOut.hpp"


BOOST_AUTO_TEST_SUITE( Uid_rate_tests );

uint64_t counter;
GlobalAddress<uint64_t> counterGA;

double wall_clock_time() {
  const double nano = 1.0e-9;
  timespec t;
  clock_gettime( CLOCK_MONOTONIC, &t );
  return nano * t.tv_nsec + t.tv_sec;
}

UIDManager * uid1;

LOOP_FUNCTOR( init_uid1_f, nid, ((int64_t,start)) ((int64_t,end)) ) {
  range_t myrange = blockDist(start, end, SoftXMT_mynode(), SoftXMT_nodes());
  uid1 = new UIDManager();
  uid1->init( myrange.start, myrange.end, node_neighbors );

  counterGA = make_global( &counter, 0 );
}

void loop_uid(int64_t st, int64_t n) {
  for ( int i=st; i<st+n; i++ ) {
    uid1->getUID();
  }
}

void loop_fa(int64_t st, int64_t n) {
  for ( int i=st; i<st+n; i++ ) {
    SoftXMT_delegate_fetch_and_add_word( counterGA, 1 );
  }
}

LOOP_FUNCTOR( test_fa_f, nid, ((int64_t,numiters)) ) {
  global_joiner.reset();
  if ( nid == 0 ) {
    async_parallel_for<loop_fa, joinerSpawn<loop_fa, ASYNC_PAR_FOR_DEFAULT>, ASYNC_PAR_FOR_DEFAULT> ( 0, numiters );
  }
  global_joiner.wait();
}

LOOP_FUNCTOR( test_uid_f, nid, ((int64_t,numiters)) ) {
  global_joiner.reset();
  if ( nid == 0 ) {
    async_parallel_for<loop_uid, joinerSpawn<loop_uid, ASYNC_PAR_FOR_DEFAULT>, ASYNC_PAR_FOR_DEFAULT> ( 0, numiters );
  }
  global_joiner.wait();
}


uint64_t iters = 1<<24;
void user_main( void * args ) {
  
  // initialize
  BOOST_MESSAGE("initializing");
  init_uid1_f f1;
  f1.start = 0;
  f1.end = iters;
  fork_join_custom(&f1);

  double fa_runtime, uidm_runtime;
  {
    // perform the work with singled fetch_add counter
    double start, end;
    BOOST_MESSAGE("perform work del");
    test_fa_f tf;
    tf.numiters = iters;
    start = wall_clock_time();
    fork_join_custom(&tf); 
    end = wall_clock_time();
    fa_runtime = end - start;
  }
  
  {
    // perform the work with uid manager
    double start, end;
    BOOST_MESSAGE("perform work uidman");
    test_uid_f tf;
    tf.numiters = iters;
    start = wall_clock_time();
    fork_join_custom(&tf); 
    end = wall_clock_time();
    uidm_runtime = end - start;
  }

  double fa_rate = iters / fa_runtime;
  double uidm_rate = iters / uidm_runtime;

  DictOut dout;
  DICT_ADD( dout, fa_runtime );
  DICT_ADD( dout, uidm_runtime );
  DICT_ADD( dout, fa_rate );
  DICT_ADD( dout, uidm_rate );
  BOOST_MESSAGE( dout.toString() );
}


BOOST_AUTO_TEST_CASE( test1 ) {

  SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  SoftXMT_activate();

  DVLOG(1) << "Spawning user main Thread....";
  SoftXMT_run_user_main( &user_main, (void*)NULL );
  VLOG(5) << "run_user_main returned";
  CHECK( SoftXMT_done() );

  SoftXMT_finish( 0 );
}



BOOST_AUTO_TEST_SUITE_END();
