////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
////////////////////////////////////////////////////////////////////////

#include <boost/test/unit_test.hpp>

#include "NTMessage.hpp"

#define PRINT 0

char buf[ 1 << 20 ] = { 0 };

size_t total = 0;

namespace Grappa {
namespace impl {

template< typename T >
NTMessage<T> send_ntmessage( Core dest, T t ) {
  NTMessage<T> m(dest,t);
  LOG(INFO) << "NTMessageBase " << m;
  t();
  return m;
}

} // namespace impl
} // namespace Grappa

BOOST_AUTO_TEST_SUITE( NTMessage_tests );

// void fn() {

//   auto m = Grappa::impl::send_ntmessage( 1, [] { 
//       if( PRINT ) LOG(INFO) << "Hi with no argument!"; 
//       total++;
//     } );

//   auto mm = reinterpret_cast<char*>( &m );
//   auto mmm = decltype(m)::deserialize_and_call( mm );
  
//   BOOST_CHECK_EQUAL( mm + 8, mmm );

//   BOOST_CHECK_EQUAL( sizeof(m), 8 );

//   int i = 1234;
//   auto n = Grappa::impl::send_ntmessage( 2, [i] { 
//       if( PRINT ) LOG(INFO) << "Hi with argument " << i << "!"; 
//       total += i;
//     } );
//   auto nn = reinterpret_cast<char*>( &n );
//   auto nnn = decltype(n)::deserialize_and_call( nn );

//   BOOST_CHECK_EQUAL( nn + 16, nnn );
//   BOOST_CHECK_EQUAL( sizeof(n), 16 );

//   int arr[ 128 ] = {0};
//   auto big = Grappa::impl::send_ntmessage( 0, [arr] { 
//       if( PRINT ) LOG(INFO) << "Hi with a big array!";
//       total += sizeof(arr);
//     } );
//   auto big_p = reinterpret_cast<char*>( &big );


//   char * next1 = Grappa::impl::run_deserialize_and_call( mm );
//   BOOST_CHECK_EQUAL( next1, mm + 8 );

//   char * next2 = Grappa::impl::run_deserialize_and_call( nn );
//   BOOST_CHECK_EQUAL( next2, nn + 16 );

//   char * next3 = Grappa::impl::run_deserialize_and_call( big_p );
//   BOOST_CHECK_EQUAL( next3, big_p + 128 * sizeof(int) + 8 );
  
//   Grappa::impl::run_deserialize_and_call( nn );

// }

void foo(void) {
  LOG(INFO) << "Function pointer message handler";
  BOOST_CHECK_EQUAL( 1, 1 );
}

void bar( const char * buf, size_t size ) {
  LOG(INFO) << "Function pointer message handler with payload " << (void*) buf;
  BOOST_CHECK_EQUAL( 1, 1 );
}

BOOST_AUTO_TEST_CASE( test1 ) {

  // Grappa::init( &(boost::unit_test::framework::master_test_suite().argc),
  //               &(boost::unit_test::framework::master_test_suite().argv) );

  // Grappa::run( [] {
  //   } );

  // Grappa::finalize();

  //fn();

  Grappa::impl::send_ntmessage( 0, &foo );
    
  // Grappa::impl::send_ntmessage( 0, &bar, &buf[0], 0 );

  Grappa::impl::send_ntmessage( 0, static_cast<void(*)()>( [] {
        LOG(INFO) << "Lambda message handler " << __PRETTY_FUNCTION__;
      } ) );

  Grappa::impl::send_ntmessage( 0, [] {
      LOG(INFO) << "Lambda message handler with no args " << __PRETTY_FUNCTION__;
    } );

  Grappa::impl::send_ntmessage( 0, [] (void) -> void {
      LOG(INFO) << "Lambda message handler with void(void) args " << __PRETTY_FUNCTION__;
    } );

  int x = 0;
  Grappa::impl::send_ntmessage( 0, [x] {
      LOG(INFO) << "Lambda message handler with capture " << x;
    } );
    
  Grappa::impl::send_ntmessage( 0, [=] {
      LOG(INFO) << "Lambda message handler with unused default copy capture " << __PRETTY_FUNCTION__;
    } );

  Grappa::impl::send_ntmessage( 0, [&] {
      LOG(INFO) << "Lambda message handler with unused default reference capture " << __PRETTY_FUNCTION__;
    } );

  auto m = Grappa::impl::send_ntmessage( 0, [=] {
      LOG(INFO) << "Lambda message handler with default capture of x = " << x << ": " << __PRETTY_FUNCTION__;
    } );

  Grappa::impl::deaggregate_amessage_buffer( reinterpret_cast<char*>(&m), sizeof(m) );
  // Grappa::impl::send_ntmessage( 0, [] (const char * buf, size_t size) {
  //     LOG(INFO) << "Lambda payload message handler with payload " << (void*) buf; 
  //   }, &buf[0], 0 );
    
  // Grappa::impl::send_ntmessage( 0, [x] (const char * buf, size_t size) { 
  //     LOG(INFO) << "Lambda payload message with capture " << x << " and payload " << (void*) buf; 
  //   }, &buf[0], 0 );


}

BOOST_AUTO_TEST_SUITE_END();

