
#include <vector>
#include <algorithm>
#include <iostream>

#include "MutableHeap.hpp"

#include <boost/test/unit_test.hpp>
#include "boost_helpers.hpp"

// template< typename T >
// class MutablePriorityQueue 
// {
  
// };




BOOST_AUTO_TEST_SUITE( MutableHeap_tests );

// BOOST_AUTO_TEST_CASE( Hello )
// {
//   std::vector< int > v;

//     v.push_back( 4 );
//     v.push_back( 1 );
//     v.push_back( 7 );
//     v.push_back( 3 );
//     v.push_back( 2 );

//     int n = 0;
//     std::cout << "[ ";
//     for(  std::vector< int >::iterator i = v.begin(); i != v.end(); ++i, ++n ) { std::cout << n << ":" << *i << " "; }
//     std::cout << "]" << std::endl;

//     softxmt::make_heap( v.begin(), v.end() );

//     n = 0;
//     std::cout << "[ ";
//     for(  std::vector< int >::iterator i = v.begin(); i != v.end(); ++i, ++n ) { std::cout << n << ":" << *i << " "; }
//     std::cout << "]" << std::endl;

//     v.push_back( 44 ); softxmt::push_heap( v.begin(), v.end() );
//     n = 0;
//     std::cout << "[ ";
//     for(  std::vector< int >::iterator i = v.begin(); i != v.end(); ++i, ++n ) { std::cout << n << ":" << *i << " "; }
//     std::cout << "]" << std::endl;


//     n = 0;
//     std::cout << "[ ";
//     for(  std::vector< int >::iterator i = v.begin(); i != v.end(); ++i, ++n ) { std::cout << n << ":" << *i << " "; }
//     std::cout << "]" << std::endl;

// }


BOOST_AUTO_TEST_CASE( MyHeap1 )
{
  MutableHeap<int,int> mh;
  mh.insert( 2345, 4 );
  BOOST_CHECK( mh.check() );
}

BOOST_AUTO_TEST_CASE( MyHeap2 )
{
  MutableHeap<int,int> mh;
  mh.insert( 2345, 4 );
  mh.insert( 1234, 1 );
  BOOST_CHECK( mh.check() );
}

BOOST_AUTO_TEST_CASE( MyHeap3 )
{
  MutableHeap<int,int> mh;
  mh.insert( 2345, 1 );
  mh.insert( 1234, 4 );
  BOOST_CHECK( mh.check() );
}

BOOST_AUTO_TEST_CASE( MyHeap4 )
{
  MutableHeap<int,int> mh;
  mh.insert( 2345, 4 );
  mh.insert( 1234, 1 );
  BOOST_CHECK( mh.check() );
  mh.remove();
  BOOST_CHECK( mh.check() );
}

BOOST_AUTO_TEST_CASE( MyHeap123 )
{
  MutableHeap<int,int> mh;
  mh.insert( 1, 1 );
  mh.insert( 2, 2 );
  mh.insert( 3, 3 );
  BOOST_CHECK( mh.check() );
  mh.dump();
  mh.remove();
  BOOST_CHECK( mh.check() );
}

BOOST_AUTO_TEST_CASE( MyHeap132 )
{
  MutableHeap<int,int> mh;
  mh.insert( 1, 1 );
  mh.insert( 3, 3 );
  mh.insert( 2, 2 );
  BOOST_CHECK( mh.check() );
  mh.dump();
  mh.remove();
  BOOST_CHECK( mh.check() );
}

BOOST_AUTO_TEST_CASE( MyHeap231 )
{
  MutableHeap<int,int> mh;
  mh.insert( 2, 2 );
  mh.insert( 3, 3 );
  mh.insert( 1, 1 );
  BOOST_CHECK( mh.check() );
  mh.dump();
  mh.remove();
  BOOST_CHECK( mh.check() );
}

BOOST_AUTO_TEST_CASE( MyHeap213 )
{
  MutableHeap<int,int> mh;
  mh.insert( 2, 2 );
  mh.insert( 1, 1 );
  mh.insert( 3, 3 );
  BOOST_CHECK( mh.check() );
  mh.dump();
  mh.remove();
  BOOST_CHECK( mh.check() );
}

BOOST_AUTO_TEST_CASE( MyHeap321 )
{
  MutableHeap<int,int> mh;
  mh.insert( 3, 3 );
  mh.insert( 2, 2 );
  mh.insert( 1, 1 );
  BOOST_CHECK( mh.check() );
  mh.dump();
  mh.remove();
  BOOST_CHECK( mh.check() );
}


BOOST_AUTO_TEST_CASE( MyHeap312 )
{
  MutableHeap<int,int> mh;
  mh.insert( 3, 3 );
  mh.insert( 1, 1 );
  mh.insert( 2, 2 );
  BOOST_CHECK( mh.check() );
  mh.dump();
  mh.remove();
  BOOST_CHECK( mh.check() );
}





BOOST_AUTO_TEST_CASE( MyHeapX )
{
  MutableHeap<int,int> mh;
  mh.insert( 2345, 4 );
  mh.insert( 1234, 1 );
  mh.insert( 3456, 7 );
  mh.insert( 4567, 2 );
  mh.insert( 5678, 3 );
  mh.dump();
  BOOST_CHECK( mh.check() );
  BOOST_CHECK_EQUAL( mh.top(), 7 );

  mh.remove();
  mh.dump();
  BOOST_CHECK( mh.check() );
  BOOST_CHECK_EQUAL( mh.top(), 4 );
}



BOOST_AUTO_TEST_CASE( MyHeapUpdate )
{
  MutableHeap<int,int> mh;
  mh.insert( 2345, 5 );
  mh.insert( 1234, 1 );
  mh.insert( 3456, 7 );
  mh.insert( 4567, 2 );
  mh.insert( 5678, 3 );
  mh.dump();
  BOOST_CHECK( mh.check() );
  BOOST_CHECK_EQUAL( mh.top(), 7 );

  mh.increase( 4567, 8 );
  mh.dump();
  BOOST_CHECK( mh.check() );
  BOOST_CHECK_EQUAL( mh.top(), 8 );

  mh.update( 3456, 4 );
  mh.dump();
  BOOST_CHECK( mh.check() );
  BOOST_CHECK_EQUAL( mh.top(), 8 );
}

BOOST_AUTO_TEST_CASE( MyHeapIdent )
{
  MutableHeap<int,int> mh;
  mh.insert( 2345, 5 );
  mh.insert( 1234, 5 );
  mh.insert( 3456, 5 );
  mh.insert( 4567, 5 );
  mh.dump();
  BOOST_CHECK( mh.check() );
  BOOST_CHECK_EQUAL( mh.top(), 5 );
  BOOST_CHECK_EQUAL( mh.top_key(), 4567 );

  mh.update( 1234, 4 );
  mh.dump();
  BOOST_CHECK( mh.check() );
  BOOST_CHECK_EQUAL( mh.top(), 5 );

}

BOOST_AUTO_TEST_SUITE_END();
