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
#include "NTMessage_devel.hpp"

// make check-NTMessage_tests

DECLARE_string( vmodule );

#define PRINT 0

char buf[ 1 << 20 ] = { 0 };

size_t total = 0;

namespace Grappa {
namespace impl {

template< typename T >
NTMessage<T> send_old_ntmessage( Core dest, T t ) {
  NTMessage<T> m(dest,t);
  LOG(INFO) << "NTMessageBase " << m;
  t();
  return m;
}

} // namespace impl
} // namespace Grappa

BOOST_AUTO_TEST_SUITE( NTMessage_tests );

// void fn() {

//   auto m = Grappa::impl::send_old_ntmessage( 1, [] { 
//       if( PRINT ) LOG(INFO) << "Hi with no argument!"; 
//       total++;
//     } );

//   auto mm = reinterpret_cast<char*>( &m );
//   auto mmm = decltype(m)::deserialize_and_call( mm );
  
//   BOOST_CHECK_EQUAL( mm + 8, mmm );

//   BOOST_CHECK_EQUAL( sizeof(m), 8 );

//   int i = 1234;
//   auto n = Grappa::impl::send_old_ntmessage( 2, [i] { 
//       if( PRINT ) LOG(INFO) << "Hi with argument " << i << "!"; 
//       total += i;
//     } );
//   auto nn = reinterpret_cast<char*>( &n );
//   auto nnn = decltype(n)::deserialize_and_call( nn );

//   BOOST_CHECK_EQUAL( nn + 16, nnn );
//   BOOST_CHECK_EQUAL( sizeof(n), 16 );

//   int arr[ 128 ] = {0};
//   auto big = Grappa::impl::send_old_ntmessage( 0, [arr] { 
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

struct Baz {
  void operator()() {
    LOG(INFO) << "Struct operator message handler";
  }
  template< typename P >
  void operator()(P * payload, size_t size) {
    LOG(INFO) << "Struct operator message handler with payload";
  }
  template< typename T, typename P >
  void operator()(T * t, P * payload, size_t size) {
    LOG(INFO) << "Struct operator message handler with pointer and payload";
  }
  template< typename T, typename P >
  void operator()(T & t, P * payload, size_t size) {
    LOG(INFO) << "Struct operator message handler with reference and payload";
  }
};

BOOST_AUTO_TEST_CASE( test1 ) {
  //FLAGS_vmodule="NTBuffer*=5";
  google::ParseCommandLineFlags( &(boost::unit_test::framework::master_test_suite().argc),
                                 &(boost::unit_test::framework::master_test_suite().argv),
                                 true);
  google::InitGoogleLogging( boost::unit_test::framework::master_test_suite().argv[0] );

  //fn();

  Grappa::impl::send_old_ntmessage( 0, &foo );
    
  // Grappa::impl::send_old_ntmessage( 0, &bar, &buf[0], 0 );

  Grappa::impl::send_old_ntmessage( 0, static_cast<void(*)()>( [] {
        LOG(INFO) << "Lambda message handler " << __PRETTY_FUNCTION__;
      } ) );

  Grappa::impl::send_old_ntmessage( 0, [] {
      LOG(INFO) << "Lambda message handler with no args " << __PRETTY_FUNCTION__;
    } );

  Grappa::impl::send_old_ntmessage( 0, [] (void) -> void {
      LOG(INFO) << "Lambda message handler with void(void) args " << __PRETTY_FUNCTION__;
    } );

  int x = 0;
  Grappa::impl::send_old_ntmessage( 0, [x] {
      LOG(INFO) << "Lambda message handler with capture " << x;
    } );
    
  Grappa::impl::send_old_ntmessage( 0, [=] {
      LOG(INFO) << "Lambda message handler with unused default copy capture " << __PRETTY_FUNCTION__;
    } );

  Grappa::impl::send_old_ntmessage( 0, [&] {
      LOG(INFO) << "Lambda message handler with unused default reference capture " << __PRETTY_FUNCTION__;
    } );

  auto m = Grappa::impl::send_old_ntmessage( 0, [=] {
      LOG(INFO) << "Lambda message handler with default capture of x = " << x << ": " << __PRETTY_FUNCTION__;
    } );

  Grappa::impl::deaggregate_nt_buffer( reinterpret_cast<char*>(&m), sizeof(m) );
  // Grappa::impl::send_old_ntmessage( 0, [] (const char * buf, size_t size) {
  //     LOG(INFO) << "Lambda payload message handler with payload " << (void*) buf; 
  //   }, &buf[0], 0 );
    
  // Grappa::impl::send_old_ntmessage( 0, [x] (const char * buf, size_t size) { 
  //     LOG(INFO) << "Lambda payload message with capture " << x << " and payload " << (void*) buf; 
  //   }, &buf[0], 0 );

  // check new message format
  {
    //
    // no payload, no address
    //
    {
      Grappa::send_new_ntmessage( 0, [] {
        LOG(INFO) << "Lambda message handler with no args and no capture " << __PRETTY_FUNCTION__;
      } );

      char c = 'x';
      Grappa::send_new_ntmessage( 0, [c] {
        LOG(INFO) << "Lambda message handler with no args and 1-byte capture " << __PRETTY_FUNCTION__;
      } );
      
      char d[4] = {'x'};
      Grappa::send_new_ntmessage( 0, [d] {
        LOG(INFO) << "Lambda message handler with no args and 4-byte capture " << __PRETTY_FUNCTION__;
      } );
      
      char e[16] = {'x'};
      Grappa::send_new_ntmessage( 0, [e] {
        LOG(INFO) << "Lambda message handler with no args and 16-byte capture " << __PRETTY_FUNCTION__;
      } );

      struct Blah {
        void operator()() {
          LOG(INFO) << __PRETTY_FUNCTION__;
        }
      };
      Grappa::send_new_ntmessage( 0, Blah() );
    }
    
    //
    // address, no payload
    //
    {
      int x = 0;
      char cx = 'x';
      
      Grappa::send_new_ntmessage( make_global( &x ), [] (int * xp) {
        ;
      } );

      Grappa::send_new_ntmessage( make_global( &x ), [] (int & xr) {
        ;
      } );

      Grappa::send_new_ntmessage( make_global( &x ), [x] (int * xp) {
        ;
      } );

      Grappa::send_new_ntmessage( make_global( &x ), [x] (int & xr) {
        ;
      } );

      struct BlahPointer {
        void operator()(int * xp) {
          LOG(INFO) << __PRETTY_FUNCTION__;
        }
      };
      Grappa::send_new_ntmessage( make_global( &x ), BlahPointer() );

      struct BlahReference {
        void operator()(int & xr) {
          LOG(INFO) << __PRETTY_FUNCTION__;
        }
      };
      Grappa::send_new_ntmessage( make_global( &x ), BlahReference() );

      struct BlahPointerConst {
        void operator()(int * xp) const {
          LOG(INFO) << __PRETTY_FUNCTION__;
        }
      };
      Grappa::send_new_ntmessage( make_global( &x ), BlahPointerConst() );

      struct BlahReferenceConst {
        void operator()(int & xr) const {
          LOG(INFO) << __PRETTY_FUNCTION__;
        }
      };
      Grappa::send_new_ntmessage( make_global( &x ), BlahReferenceConst() );
    }

    //
    // payload, no address
    //
    {
      const int payload_count = 16;
      int payload[payload_count] = {0};
      Grappa::send_new_ntmessage( 0, &payload[0], payload_count,
                                  [] (int * payload, size_t count) {
                                    ;
                                  } );

      char c = 'x';
      Grappa::send_new_ntmessage( 0, &payload[0], payload_count,
                                  [c] (int * payload, size_t count) {
                                    ;
                                  } );
      
      char d[4] = {'x'};
      Grappa::send_new_ntmessage( 0, &payload[0], payload_count,
                                  [d] (int * payload, size_t count) {
                                    ;
                                  } );
      
      char e[16] = {'x'};
      Grappa::send_new_ntmessage( 0, &payload[0], payload_count,
                                  [e] (int * payload, size_t count) {
                                    ;
                                  } );
    }

    //
    // payload with address
    //
    {
      int x = 0;
      const int payload_count = 16;
      int payload[payload_count] = {0};
      char cx = 'x';
      
      Grappa::send_new_ntmessage( make_global( &x ), &payload[0], payload_count,
                                  [] (int * xp, int * payload, size_t count) {
                                    ;
                                  } );

      Grappa::send_new_ntmessage( make_global( &x ), &payload[0], payload_count,
                                  [] (int & xr, int * payload, size_t count) {
                                    ;
                                  } );

      char c = 'x';
      Grappa::send_new_ntmessage( make_global( &x ), &payload[0], payload_count,
                                  [c] (int * xp, int * payload, size_t count) {
                                    ;
                                  } );
      
      Grappa::send_new_ntmessage( make_global( &x ), &payload[0], payload_count,
                                  [c] (int & xr, int * payload, size_t count) {
                                    ;
                                  } );
      
      struct BlahPointerPayload {
        void operator()(int * xp, int * payload, size_t count) {
          LOG(INFO) << __PRETTY_FUNCTION__;
        }
      };
      Grappa::send_new_ntmessage( make_global( &x ), &payload[0], payload_count, BlahPointerPayload() );

      struct BlahReferencePayload {
        void operator()(int & xr, int * payload, size_t count) {
          LOG(INFO) << __PRETTY_FUNCTION__;
        }
      };
      Grappa::send_new_ntmessage( make_global( &x ), &payload[0], payload_count, BlahReferencePayload() );

      struct BlahPointerPayloadConst {
        void operator()(int * xp, int * payload, size_t count) const {
          LOG(INFO) << __PRETTY_FUNCTION__;
        }
      };
      Grappa::send_new_ntmessage( make_global( &x ), &payload[0], payload_count, BlahPointerPayloadConst() );

      struct BlahReferencePayloadConst {
        void operator()(int & xr, int * payload, size_t count) const {
          LOG(INFO) << __PRETTY_FUNCTION__;
        }
      };
      Grappa::send_new_ntmessage( make_global( &x ), &payload[0], payload_count, BlahReferencePayloadConst() );
    }
  }

  //
  // test aggregation/deaggregation with new NTMessage specializers
  // (run with make -j4 check-NTMessage_tests)
  // 
  LOG(INFO) << "test aggregation/deaggregation with new NTMessage specializers";

  auto deserialize_helper = [] ( Grappa::impl::NTBuffer * b ) {
      auto buftuple = b->take_buffer();
      char * buf = static_cast< char * >( std::get<0>(buftuple) );
      size_t size = std::get<1>(buftuple);

      char * end = Grappa::impl::deaggregate_new_nt_buffer( buf, size );
      BOOST_CHECK_EQUAL( end, buf + size );
      
      Grappa::impl::NTBuffer::free_buffer( buf );
  };
  
  { // no capture, no payload, no address
    Grappa::impl::NTBuffer ntbuf;

    static int i = 0;
    auto f = [] { ++i; };

    // send some compressable messages
    Grappa::impl::NTMessageSpecializer< decltype(f) >::send_ntmessage( 0, f, &ntbuf );
    Grappa::impl::NTMessageSpecializer< decltype(f) >::send_ntmessage( 0, f, &ntbuf );
    Grappa::impl::NTMessageSpecializer< decltype(f) >::send_ntmessage( 0, f, &ntbuf );
    Grappa::impl::NTMessageSpecializer< decltype(f) >::send_ntmessage( 0, f, &ntbuf );
    Grappa::impl::NTMessageSpecializer< decltype(f) >::send_ntmessage( 0, f, &ntbuf );

    // check that compression worked as expected
    BOOST_CHECK_EQUAL( ntbuf.size(), sizeof( Grappa::impl::NTHeader ) );
    
    // send a non-compressable message
    Grappa::impl::NTMessageSpecializer< decltype(f) >::send_ntmessage( 1, f, &ntbuf );

    // check that compression worked as expected
    BOOST_CHECK_EQUAL( ntbuf.size(), 2 * sizeof( Grappa::impl::NTHeader ) );

    // deserialize and call
    deserialize_helper( &ntbuf );
    
    BOOST_CHECK_EQUAL( i, 6 );
  }

  { // no payload, no address, with capture
    Grappa::impl::NTBuffer ntbuf;

    static int i = 0;

    // Compressable messages; ensure capture changes each iteration
    const int compressed_count = 15; // large enough to cause flush
    for( int j = 0; j < compressed_count; ++j ) {
      auto ff = [j] { i += j; };
      Grappa::impl::NTMessageSpecializer< decltype(ff) >::send_ntmessage( 0, ff, &ntbuf );
    }
    
    // check that compression worked as expected
    BOOST_CHECK_EQUAL( ntbuf.size(),
                       ( sizeof( Grappa::impl::NTHeader ) +
                         compressed_count * sizeof(int) ) );

    // Non-compressable message
    int increment = 1;
    auto f = [increment] { i += increment; };
    Grappa::impl::NTMessageSpecializer< decltype(f) >::send_ntmessage( 0, f, &ntbuf );

    // check that compression worked as expected
    BOOST_CHECK_EQUAL( ntbuf.size(),
                       ( 2 * sizeof( Grappa::impl::NTHeader ) +
                         (1 + compressed_count) * sizeof(int) + // payloads
                         4 ) ); // padding for 16-byte alignment of second header
    
    deserialize_helper( &ntbuf );

    BOOST_CHECK_EQUAL(i, 1 + compressed_count * (compressed_count-1) / 2 );
  }

  { // My plan for dealing with function pointers takes more space
    // than I want, so maybe we can avoid dealing with them in
    // messages by always wrapping them in a lambda?
    Grappa::impl::NTBuffer ntbuf;

    auto g = [] { foo(); };
    Grappa::impl::NTMessageSpecializer< decltype(g) >::send_ntmessage( 0, g, &ntbuf );

    deserialize_helper( &ntbuf );
  }

  // Test for no-payload no-capture message with address with pointer argument
  {
    Grappa::impl::NTBuffer ntbuf;

    int i = 0;
    auto global_i = make_global(&i,0);
    auto f = [] (int * i) { ++(*i); };

    Grappa::impl::NTAddressMessageSpecializer< decltype(f), int>::send_ntmessage( global_i, f, &ntbuf );
    Grappa::impl::NTAddressMessageSpecializer< decltype(f), int>::send_ntmessage( global_i, f, &ntbuf );
    Grappa::impl::NTAddressMessageSpecializer< decltype(f), int>::send_ntmessage( global_i, f, &ntbuf );
    Grappa::impl::NTAddressMessageSpecializer< decltype(f), int>::send_ntmessage( global_i, f, &ntbuf );
    Grappa::impl::NTAddressMessageSpecializer< decltype(f), int>::send_ntmessage( global_i, f, &ntbuf );

    deserialize_helper( &ntbuf );

    BOOST_CHECK_EQUAL(i,5);
  }

  // Test for no-payload no-capture message with address with reference argument
  {
    Grappa::impl::NTBuffer ntbuf;

    int i = 0;
    auto global_i = make_global(&i,0);
    auto f = [] (int & target) { ++target; };

    Grappa::impl::NTAddressMessageSpecializer< decltype(f), int >::send_ntmessage( global_i, f, &ntbuf );
    Grappa::impl::NTAddressMessageSpecializer< decltype(f), int >::send_ntmessage( global_i, f, &ntbuf );
    Grappa::impl::NTAddressMessageSpecializer< decltype(f), int >::send_ntmessage( global_i, f, &ntbuf );
    Grappa::impl::NTAddressMessageSpecializer< decltype(f), int >::send_ntmessage( global_i, f, &ntbuf );
    Grappa::impl::NTAddressMessageSpecializer< decltype(f), int >::send_ntmessage( global_i, f, &ntbuf );

    deserialize_helper( &ntbuf );

    BOOST_CHECK_EQUAL(i,5);
  }

  // Test for no-payload message with address and capture with pointer argument
  {
    Grappa::impl::NTBuffer ntbuf;

    int i = 0;
    int increment = 1;
    auto global_i = make_global(&i,0);
    auto f = [increment] (int * i) { (*i) += increment; };

    Grappa::impl::NTAddressMessageSpecializer< decltype(f), int >::send_ntmessage( global_i, f, &ntbuf );
    Grappa::impl::NTAddressMessageSpecializer< decltype(f), int >::send_ntmessage( global_i, f, &ntbuf );
    Grappa::impl::NTAddressMessageSpecializer< decltype(f), int >::send_ntmessage( global_i, f, &ntbuf );
    Grappa::impl::NTAddressMessageSpecializer< decltype(f), int >::send_ntmessage( global_i, f, &ntbuf );
    Grappa::impl::NTAddressMessageSpecializer< decltype(f), int >::send_ntmessage( global_i, f, &ntbuf );

    deserialize_helper( &ntbuf );

    BOOST_CHECK_EQUAL(i,5);
  }

// Test for no-payload message with address and capture with reference argument
  {
    Grappa::impl::NTBuffer ntbuf;

    int i = 0;
    int increment = 1;
    auto global_i = make_global(&i,0);
    auto f = [increment] (int & target) { target += increment; };

    Grappa::impl::NTAddressMessageSpecializer< decltype(f), int >::send_ntmessage( global_i, f, &ntbuf );
    Grappa::impl::NTAddressMessageSpecializer< decltype(f), int >::send_ntmessage( global_i, f, &ntbuf );
    Grappa::impl::NTAddressMessageSpecializer< decltype(f), int >::send_ntmessage( global_i, f, &ntbuf );
    Grappa::impl::NTAddressMessageSpecializer< decltype(f), int >::send_ntmessage( global_i, f, &ntbuf );
    Grappa::impl::NTAddressMessageSpecializer< decltype(f), int >::send_ntmessage( global_i, f, &ntbuf );

    deserialize_helper( &ntbuf );

    BOOST_CHECK_EQUAL(i,5);
  }

  // Test for no-capture no-address message with payload
  {
    Grappa::impl::NTBuffer ntbuf;

    static int i = 0;
    int payload = 5;
    auto f = [] ( int * payload, size_t size ) { i += *payload; };

    Grappa::impl::NTPayloadMessageSpecializer< decltype(f), int >::send_ntmessage( 0, &payload, sizeof(payload), f, &ntbuf );
    Grappa::impl::NTPayloadMessageSpecializer< decltype(f), int >::send_ntmessage( 0, &payload, sizeof(payload), f, &ntbuf );
    Grappa::impl::NTPayloadMessageSpecializer< decltype(f), int >::send_ntmessage( 0, &payload, sizeof(payload), f, &ntbuf );
    Grappa::impl::NTPayloadMessageSpecializer< decltype(f), int >::send_ntmessage( 0, &payload, sizeof(payload), f, &ntbuf );
    Grappa::impl::NTPayloadMessageSpecializer< decltype(f), int >::send_ntmessage( 0, &payload, sizeof(payload), f, &ntbuf );
    
    deserialize_helper( &ntbuf );

    BOOST_CHECK_EQUAL(i,25);
  }

  { // no-address, payload, capture
    LOG(INFO) << "no-address, paload, capture";
    Grappa::impl::NTBuffer ntbuf;
    
    static int i = 0;
    int increment = 1;
    int payload = 2;
    auto f = [increment] (int * payload, size_t size) {
      i += increment + *payload;
    };

    Grappa ::impl::NTPayloadMessageSpecializer< decltype(f), int>::send_ntmessage( 0, &payload, sizeof(payload), f, &ntbuf );
    Grappa ::impl::NTPayloadMessageSpecializer< decltype(f), int>::send_ntmessage( 0, &payload, sizeof(payload), f, &ntbuf );
    Grappa ::impl::NTPayloadMessageSpecializer< decltype(f), int>::send_ntmessage( 0, &payload, sizeof(payload), f, &ntbuf );
    Grappa ::impl::NTPayloadMessageSpecializer< decltype(f), int>::send_ntmessage( 0, &payload, sizeof(payload), f, &ntbuf );
    Grappa ::impl::NTPayloadMessageSpecializer< decltype(f), int>::send_ntmessage( 0, &payload, sizeof(payload), f, &ntbuf );

    deserialize_helper( &ntbuf );

    BOOST_CHECK_EQUAL( i, 15 );
    // ntbuf.size()
  }

  { // no capture, payload, address
    LOG(INFO) << "no capture, payload, address, lambda takes a pointer";
    Grappa::impl::NTBuffer ntbuf;

    int64_t i = 0;
    auto global_i = make_global( &i, 0 );
    int payload = 1;
    auto f = [] ( int64_t * i, int * payload, size_t size ) {
      *i += *payload;
    };

    Grappa::impl::NTPayloadAddressMessageSpecializer< decltype(f), int64_t, int >::send_ntmessage( global_i, &payload, 1, f, &ntbuf );
    Grappa::impl::NTPayloadAddressMessageSpecializer< decltype(f), int64_t, int >::send_ntmessage( global_i, &payload, 1, f, &ntbuf );
    Grappa::impl::NTPayloadAddressMessageSpecializer< decltype(f), int64_t, int >::send_ntmessage( global_i, &payload, 1, f, &ntbuf );
    Grappa::impl::NTPayloadAddressMessageSpecializer< decltype(f), int64_t, int >::send_ntmessage( global_i, &payload, 1, f, &ntbuf );
    Grappa::impl::NTPayloadAddressMessageSpecializer< decltype(f), int64_t, int >::send_ntmessage( global_i, &payload, 1, f, &ntbuf );

    deserialize_helper( &ntbuf );

    BOOST_CHECK_EQUAL( i, 5 );
  }

  { // no capture, payload, address, lambda takes a reference
    LOG(INFO) << "no capture, payload, address, lambda takes a pointer";
    Grappa::impl::NTBuffer ntbuf;

    int64_t i = 0;
    auto global_i = make_global( &i, 0 );
    int payload = 1;
    auto f = [] ( int64_t & i, int * payload, size_t size ) {
      i += *payload;
    };

    Grappa::impl::NTPayloadAddressMessageSpecializer< decltype(f), int64_t, int >::send_ntmessage( global_i, &payload, 1, f, &ntbuf );
    Grappa::impl::NTPayloadAddressMessageSpecializer< decltype(f), int64_t, int >::send_ntmessage( global_i, &payload, 1, f, &ntbuf );
    Grappa::impl::NTPayloadAddressMessageSpecializer< decltype(f), int64_t, int >::send_ntmessage( global_i, &payload, 1, f, &ntbuf );
    Grappa::impl::NTPayloadAddressMessageSpecializer< decltype(f), int64_t, int >::send_ntmessage( global_i, &payload, 1, f, &ntbuf );
    Grappa::impl::NTPayloadAddressMessageSpecializer< decltype(f), int64_t, int >::send_ntmessage( global_i, &payload, 1, f, &ntbuf );

    deserialize_helper( &ntbuf );

    BOOST_CHECK_EQUAL( i, 5 );
  }

  { // capture, payload, address
    LOG(INFO) << "capture, payload, address";
    Grappa::impl::NTBuffer ntbuf;

    int64_t i = 0;
    auto global_i = make_global( &i, 0 );
    int64_t increment = 1;
    int payload   = 2;
    auto f = [increment] ( int64_t * i, int * payload, size_t size ) {
      *i += increment + *payload;
    };

    // send some messages
    Grappa::impl::NTPayloadAddressMessageSpecializer< decltype(f), int64_t, int >::send_ntmessage( global_i, &payload, 1, f, &ntbuf );
    Grappa::impl::NTPayloadAddressMessageSpecializer< decltype(f), int64_t, int >::send_ntmessage( global_i, &payload, 1, f, &ntbuf );
    Grappa::impl::NTPayloadAddressMessageSpecializer< decltype(f), int64_t, int >::send_ntmessage( global_i, &payload, 1, f, &ntbuf );
    Grappa::impl::NTPayloadAddressMessageSpecializer< decltype(f), int64_t, int >::send_ntmessage( global_i, &payload, 1, f, &ntbuf );
    Grappa::impl::NTPayloadAddressMessageSpecializer< decltype(f), int64_t, int >::send_ntmessage( global_i, &payload, 1, f, &ntbuf );

    // deserialize and call
    deserialize_helper( &ntbuf );
    
    BOOST_CHECK_EQUAL( i, 15 );
  }

  { // capture, payload, address; lambda takes reference
    LOG(INFO) << "capture, payload, address w/ reference";
    Grappa::impl::NTBuffer ntbuf;

    int64_t i = 0;
    auto global_i = make_global( &i, 0 );
    int64_t increment = 1;
    int payload       = 2;
    auto f = [increment] ( int64_t& i, int * payload, size_t size ) {
      i += increment + *payload;
    };

    // send some messages
    Grappa::impl::NTPayloadAddressMessageSpecializer< decltype(f), int64_t, int >::send_ntmessage( global_i, &payload, 1, f, &ntbuf );
    Grappa::impl::NTPayloadAddressMessageSpecializer< decltype(f), int64_t, int >::send_ntmessage( global_i, &payload, 1, f, &ntbuf );
    Grappa::impl::NTPayloadAddressMessageSpecializer< decltype(f), int64_t, int >::send_ntmessage( global_i, &payload, 1, f, &ntbuf );
    Grappa::impl::NTPayloadAddressMessageSpecializer< decltype(f), int64_t, int >::send_ntmessage( global_i, &payload, 1, f, &ntbuf );
    Grappa::impl::NTPayloadAddressMessageSpecializer< decltype(f), int64_t, int >::send_ntmessage( global_i, &payload, 1, f, &ntbuf );

    // deserialize and call
    deserialize_helper( &ntbuf );
    
    BOOST_CHECK_EQUAL( i, 15 );
  }
}

BOOST_AUTO_TEST_SUITE_END();

