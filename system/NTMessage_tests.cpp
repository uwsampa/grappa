////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
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

  Grappa::impl::deaggregate_nt_buffer( reinterpret_cast<char*>(&m), sizeof(m) );
  // Grappa::impl::send_ntmessage( 0, [] (const char * buf, size_t size) {
  //     LOG(INFO) << "Lambda payload message handler with payload " << (void*) buf; 
  //   }, &buf[0], 0 );
    
  // Grappa::impl::send_ntmessage( 0, [x] (const char * buf, size_t size) { 
  //     LOG(INFO) << "Lambda payload message with capture " << x << " and payload " << (void*) buf; 
  //   }, &buf[0], 0 );


}

BOOST_AUTO_TEST_SUITE_END();

