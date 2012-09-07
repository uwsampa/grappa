#include <boost/test/unit_test.hpp>

#include "SoftXMT.hpp"
#include "Uid.hpp"
#include "Collective.hpp"
#include "common.hpp"
#include "ForkJoin.hpp"
#include "AsyncParallelFor.hpp"
#include "GlobalTaskJoiner.hpp"


BOOST_AUTO_TEST_SUITE( Uid_tests );

UIDManager * uid1;
UIDManager * uid2;

uint64_t local_total1 = 0;
uint64_t local_total2 = 0;
uint64_t all_total1;
uint64_t all_total2;

LOOP_FUNCTOR( init_uid1_f, nid, ((int64_t,start)) ((int64_t,end)) ) {
  range_t myrange = blockDist(start, end, SoftXMT_mynode(), SoftXMT_nodes());
  uid1 = new UIDManager();
  uid1->init( myrange.start, myrange.end, node_neighbors );
}

LOOP_FUNCTOR( init_uid2_f, nid, ((int64_t,start)) ((int64_t,end)) ) {
  range_t myrange = blockDist(start, end, SoftXMT_mynode(), SoftXMT_nodes());
  uid2 = new UIDManager();
  uid2->init( myrange.start, myrange.end, node_neighbors );
}

void loop(int64_t st, int64_t n) {
  for ( int i=st; i<st+n; i++ ) {
    //BOOST_MESSAGE("iteration=" << i);
    local_total1 += uid1->getUID();

    local_total2 += uid2->getUID();
    local_total2 += uid2->getUID();
    //BOOST_MESSAGE("iteration=" << i << " done");
  }
}

LOOP_FUNCTOR( test_f, nid, ((int64_t,numiters)) ) {
  global_joiner.reset();
  if ( nid == 0 ) {
    async_parallel_for<loop, joinerSpawn<loop, ASYNC_PAR_FOR_DEFAULT>, ASYNC_PAR_FOR_DEFAULT> ( 0, numiters );
  }
  global_joiner.wait();
}

LOOP_FUNCTOR( reduce_f, nid, ((uint64_t*,valaddr)) ((uint64_t*,all_total_addr)) ) {
  int64_t total_cast = *valaddr;
  *all_total_addr = SoftXMT_allreduce<int64_t,COLL_ADD,0>(total_cast);
}


uint64_t iters = 1<<20;
void user_main( void * args ) {
  
  // initialize uid 1
  BOOST_MESSAGE("initializing uid1");
  init_uid1_f f1;
  f1.start = 0;
  f1.end = iters;
  fork_join_custom(&f1);

  // initialize uid2
  BOOST_MESSAGE("initializing uid2");
  uint64_t iters2 = iters*2;
  init_uid2_f f2;
  f2.start = 0;
  f2.end = iters2;
  fork_join_custom(&f2);

  // perform the work (grabbing uids)
  BOOST_MESSAGE("perform work");
  test_f tf;
  tf.numiters = iters;
  fork_join_custom(&tf); 

  // reduce the check values
  BOOST_MESSAGE("check values");
  reduce_f rf1;
  rf1.valaddr = &local_total1;
  rf1.all_total_addr = &all_total1;
  fork_join_custom(&rf1);
  reduce_f rf2;
  rf2.valaddr = &local_total2;
  rf2.all_total_addr = &all_total2;
  fork_join_custom(&rf2);

  // (0+1+2+...+(iters-1)) = (iters-1)*iters/2
  // (0+1+2+...+(2*iters-1))
  uint64_t expected1 = ((iters-1) * iters) / 2;
  uint64_t expected2 = ((iters2-1) * iters2) / 2;

  BOOST_MESSAGE( "all_total1=" << all_total1 << " expected1=" << expected1 );
  BOOST_CHECK( all_total1 == expected1 );
  
  BOOST_MESSAGE( "all_total2=" << all_total2 << " expected2=" << expected2 );
  BOOST_CHECK( all_total2 == expected2 );
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
