
#include <boost/test/unit_test.hpp>

#include "SoftXMT.hpp"
#include "Delegate.hpp"

BOOST_AUTO_TEST_SUITE( Delegate_tests );


FUNCTOR( AtomicAddDouble, ((GlobalAddress<double>,addr)) ((double,value)) ) {
  CHECK(addr.node() == SoftXMT_mynode());
  *addr.pointer() += value;
}

int64_t some_data = 1234;

double some_double = 123.0;

int64_t other_data __attribute__ ((aligned (2048))) = 0;

// For generic template-based delegate
struct Pair {
  int64_t left;
  int64_t right;

  Pair( int64_t l, int64_t r )
    : left( l )
    , right( r ) { }
};
Pair pair_genericTest( 100, 200 );
struct ArgumentPair {
  int64_t inc1;
  int64_t inc2;
  GlobalAddress<Pair> addr;
};
struct ResultPair {
  int64_t res1;
  int64_t res2;
};

ResultPair fetchAddPair( ArgumentPair ap ) {
  CHECK( ap.addr.node() == SoftXMT_mynode() );
  Pair * p = ap.addr.pointer();

  ResultPair rp;
  rp.res1 = p->left;
  rp.res2 = p->right;

  p->left += ap.inc1;
  p->right += ap.inc2;

  return rp;
}


void user_main( int * args ) 
{
  BOOST_CHECK_EQUAL( 2, SoftXMT_nodes() );
  // try read
  some_data = 1111;
  int64_t remote_data = SoftXMT_delegate_read_word( make_global(&some_data,1) );
  BOOST_CHECK_EQUAL( 1234, remote_data );
  BOOST_CHECK_EQUAL( 1111, some_data );
  
  // write
  SoftXMT_delegate_write_word( make_global(&some_data,1), 2345 );
  BOOST_CHECK_EQUAL( 1111, some_data );
  
  // verify write
  remote_data = SoftXMT_delegate_read_word( make_global(&some_data,1) );
  BOOST_CHECK_EQUAL( 2345, remote_data );

  // fetch and add
  remote_data = SoftXMT_delegate_fetch_and_add_word( make_global(&some_data,1), 1 );
  BOOST_CHECK_EQUAL( 1111, some_data );
  BOOST_CHECK_EQUAL( 2345, remote_data );
  
  // verify write
  remote_data = SoftXMT_delegate_read_word( make_global(&some_data,1) );
  BOOST_CHECK_EQUAL( 2346, remote_data );

  // check compare_and_swap
  bool swapped;
  swapped = SoftXMT_delegate_compare_and_swap_word( make_global(&some_data,1), 123, 3333); // shouldn't swap
  BOOST_CHECK_EQUAL( swapped, false );
  // verify value is unchanged
  remote_data = SoftXMT_delegate_read_word( make_global(&some_data,1) );
  BOOST_CHECK_EQUAL( 2346, remote_data );
  
  // now actually do swap
  swapped = SoftXMT_delegate_compare_and_swap_word( make_global(&some_data,1), 2346, 3333);
  BOOST_CHECK_EQUAL( swapped, true );
  // verify value is changed
  remote_data = SoftXMT_delegate_read_word( make_global(&some_data,1) );
  BOOST_CHECK_EQUAL( 3333, remote_data );
  
  // try linear global address

  // initialize
  other_data = 0;
  SoftXMT_delegate_write_word( make_global(&other_data,1), 1 );

  int * foop = new int;
  *foop = 1234;
  BOOST_MESSAGE( *foop );
    

  // make address
  BOOST_MESSAGE( "pointer is " << &other_data );
  GlobalAddress< int64_t > la = make_linear( &other_data );

  // check pointer computation
  BOOST_CHECK_EQUAL( la.node(), 0 );
  BOOST_CHECK_EQUAL( la.pointer(), &other_data );

  // check data
  BOOST_CHECK_EQUAL( 0, other_data );
  remote_data = SoftXMT_delegate_read_word( la );
  BOOST_CHECK_EQUAL( 0, remote_data );

  // change pointer and check computation
  ++la;
  BOOST_CHECK_EQUAL( la.node(), 0 );
  BOOST_CHECK_EQUAL( la.pointer(), &other_data + 1 );

  // change pointer and check computation
  la += 7;
  BOOST_CHECK_EQUAL( la.node(), 1 );
  BOOST_CHECK_EQUAL( la.pointer(), &other_data );

  // check remote data
  remote_data = SoftXMT_delegate_read_word( la );
  BOOST_CHECK_EQUAL( 1, remote_data );
  


  // check template read
  // try read
  remote_data = 1234;
  SoftXMT_delegate_read< int64_t >( make_global(&some_data,1), &remote_data );
  BOOST_CHECK_EQUAL( 3333, remote_data );


  // check delegate func
  GlobalAddress<double> other_double = make_global(&some_double, 1);
  AtomicAddDouble aad(other_double, 12.0);
  SoftXMT_delegate_func(&aad, aad.addr.node());
  
  double chk_double;
  SoftXMT_delegate_read(other_double, &chk_double);
  BOOST_CHECK_EQUAL( chk_double, some_double+12.0 );


  ///
  /// generic template-based delegate
  ///
  ArgumentPair ap;
  ap.inc1 = 1;
  ap.inc2 = 2;
  ap.addr = make_global( &pair_genericTest, 1 );
  ResultPair rp = SoftXMT_delegate_func<ArgumentPair,ResultPair,fetchAddPair>( ap, 1 );
  // check return value
  BOOST_CHECK_EQUAL( rp.res1, 100 );
  BOOST_CHECK_EQUAL( rp.res2, 200 );
  // check memory location
  int64_t src_left = SoftXMT_delegate_read_word( make_global( &pair_genericTest.left, 1 ) );
  int64_t src_right = SoftXMT_delegate_read_word( make_global( &pair_genericTest.right, 1 ) );
  BOOST_CHECK_EQUAL( src_left, 101 );
  BOOST_CHECK_EQUAL( src_right, 202 );
}


BOOST_AUTO_TEST_CASE( test1 ) {

  SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  SoftXMT_activate();



  DVLOG(1) << "Spawning user main Thread....";
  SoftXMT_run_user_main( &user_main, (int*)NULL );
  BOOST_CHECK( SoftXMT_done() == true );

  SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

