
#include <boost/test/unit_test.hpp>

#include "SoftXMT.hpp"
#include "Addressing.hpp"


BOOST_AUTO_TEST_SUITE( Addressing_tests );

// template< typename T >
// T bazf( T t ) {
//   GlobalPointer< T >( t );
//   return t.pointer();
// }

BOOST_AUTO_TEST_CASE( test1 ) {

  SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  SoftXMT_activate();

  int foo;
  int bar;
  GlobalAddress< int > foop( &foo );
  GlobalAddress< int > barp( &bar );
  BOOST_CHECK_EQUAL( &foo, foop.pointer() );
  BOOST_CHECK_EQUAL( &bar, barp.pointer() );
  BOOST_CHECK_EQUAL( 8, sizeof( barp ) );

  GlobalAddress< int > gp2 = localToGlobal( &foo );
  BOOST_CHECK_EQUAL( gp2.pointer(), &foo );

  

  SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

