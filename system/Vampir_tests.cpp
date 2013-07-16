#include "Grappa.hpp"
#include <boost/test/unit_test.hpp>

#ifdef VTRACE
#include <vt_user.h>
#endif

BOOST_AUTO_TEST_SUITE( Vampir_tests );

BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
               &(boost::unit_test::framework::master_test_suite().argv)
               );
  Grappa_activate();

  // MPI_Init( &(boost::unit_test::framework::master_test_suite().argc),
  //           &(boost::unit_test::framework::master_test_suite().argv)
  //           );
  // gasnet_init( &(boost::unit_test::framework::master_test_suite().argc),
  //              &(boost::unit_test::framework::master_test_suite().argv)
  //              );
  // gasnet_attach( NULL, 0, 0, 0 );


  sleep(10);

  // define
  int counter_grp_vt = VT_COUNT_GROUP_DEF( "Vampir_tests" );
  int counter_vt = VT_COUNT_DEF( "Counter", "cnt", VT_COUNT_TYPE_UNSIGNED, counter_grp_vt );

  // start
  // VT_USER_START("sampling");

  // sample
  for( int64_t i = 0; i < 1000; ++i ) {
    //MPI_Barrier( MPI_COMM_WORLD );
    // gasnet_barrier_notify( 0, GASNET_BARRIERFLAG_ANONYMOUS );
    // gasnet_barrier_wait( 0, GASNET_BARRIERFLAG_ANONYMOUS );
    VT_COUNT_UNSIGNED_VAL( counter_vt, i );
    //MPI_Barrier( MPI_COMM_WORLD );
    // gasnet_barrier_notify( 0, GASNET_BARRIERFLAG_ANONYMOUS );
    // gasnet_barrier_wait( 0, GASNET_BARRIERFLAG_ANONYMOUS );
  }

  //if( Grappa::mycore() == 0 ) _exit(1);

  // end
  // VT_USER_END("sampling");
  // VT_BUFFER_FLUSH();

  //VT_OFF();

  Grappa_finish(0);
  //MPI_Barrier( MPI_COMM_WORLD );
  //vt_close();
  //_exit(0);
  // gasnet_exit(0);
  //gasnett_killmyprocess(0);
}

BOOST_AUTO_TEST_SUITE_END();
