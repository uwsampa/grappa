#include <boost/test/unit_test.hpp>

#include "SoftXMT.hpp"
#include "GlobalTaskJoiner.hpp"
#include "ForkJoin.hpp"
#include "AsyncParallelFor.hpp"
#include "Delegate.hpp"

BOOST_AUTO_TEST_SUITE( AsyncParallelFor_Global_tests );

#define MAX_NODES 8
#define SIZE 1000000
int64_t done[SIZE*MAX_NODES] = {0};
int64_t global_count = 0;
int64_t local_count = 0;

void loop_body(int64_t start, int64_t num ) {
  /*BOOST_MESSAGE( "execute [" << start << ","
            << start+num << ") with thread " << CURRENT_THREAD->id );*/
  for (int i=start; i<start+num; i++) {
    int64_t marked = SoftXMT_delegate_fetch_and_add_word( make_global( &done[i], 0 ), 1 );
    BOOST_CHECK( marked == 0 );

    SoftXMT_delegate_fetch_and_add_word( make_global( &global_count, 0 ), 1 ); 
    local_count++;
  }
}

struct parallel_func : public ForkJoinIteration {
  void operator()(int64_t nid) const {
    int64_t start = nid * SIZE;
    int64_t iters = SIZE;

    global_joiner.reset();
    async_parallel_for<&loop_body, &joinerSpawn<loop_body> >( start, iters ); 
    global_joiner.wait();
  }
}; 

  

void user_main( void * args ) {
  BOOST_CHECK( SoftXMT_nodes() <= MAX_NODES );
  
  int64_t total_iters = SoftXMT_nodes() * SIZE;
  BOOST_MESSAGE( total_iters << " iterations over " <<
                 SoftXMT_nodes() << " nodes" );

  parallel_func f;
  fork_join_custom(&f);

  for (int i=0; i<total_iters; i++) {
    BOOST_CHECK( done[i] == 1 );
  }
  BOOST_CHECK( global_count == total_iters );

  // print out individual participations 
  for (Node n=0; n<SoftXMT_nodes(); n++) {
    int64_t n_count = SoftXMT_delegate_read_word( make_global( &local_count, n ) );
    BOOST_MESSAGE( n << " did " << n_count << " iterations" );
  }

  BOOST_MESSAGE( "user main is exiting" );
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

