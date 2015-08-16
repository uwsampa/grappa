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

/// Tests for Aggregator. This is a bit brittle due to testing
/// timeouts.

#include "Aggregator.hpp"

// reuse flag from Aggregator.cpp
DECLARE_int64( aggregator_autoflush_ticks );

/*
 * tests
 */
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE( Aggregator_tests );


int64_t count = 0;
void function() {
  ++count;
}

struct first_call_args 
{
  int arg1;
  double arg2;
};

int first_int = 0;

void first_call(first_call_args * args, size_t args_size, void * payload, size_t payload_size) 
{
  //BOOST_MESSAGE( "first_call: arg1=" << args->arg1 << " arg2=" << args->arg2 );
  ++first_int;
}


struct second_call_args 
{
  char arg1[ 100 ];
  int64_t i;
};

int second_int = 0;

void second_call(second_call_args * args, size_t args_size, void * payload, size_t payload_size) 
{
  //BOOST_MESSAGE( "second_call: arg1=" << args->arg1 );
  second_int += args->i;
  BOOST_MESSAGE( "received i " << args-> i << " second_int " << second_int );
  BOOST_CHECK( payload_size == 0 || payload_size == args_size );
}

struct done_call_args 
{
};

bool done = false;

void done_call(done_call_args * args, size_t args_size, void * payload, size_t payload_size) 
{
  done = true;
  BOOST_MESSAGE( "received done " );
  BOOST_CHECK( payload_size == 0 || payload_size == args_size );
}


BOOST_AUTO_TEST_CASE( test1 ) {
  google::ParseCommandLineFlags( &(boost::unit_test::framework::master_test_suite().argc),
                                 &(boost::unit_test::framework::master_test_suite().argv), 
                                 true );
  google::InitGoogleLogging(boost::unit_test::framework::master_test_suite().argv[0]);
  google::InstallFailureSignalHandler( );
  FLAGS_aggregator_autoflush_ticks = 1000000;

  Communicator& s = global_communicator;
  s.init( &(boost::unit_test::framework::master_test_suite().argc),
          &(boost::unit_test::framework::master_test_suite().argv) );
  BOOST_CHECK( s.cores() >= 2 );
  BOOST_MESSAGE( "We have " << s.cores() << " nodes." );
  Aggregator& a = global_aggregator;
  a.init();

  s.activate();

  BOOST_CHECK( s.cores() >= 2 );
  if( s.mycore() == 0 ) {

  // make sure we can send something
  first_call_args first_args = { 1, 2.3 };

  // try with automagic arg size discovery
  Grappa_call_on( 0, &first_call, &first_args );

  a.flush( 0 );
  a.poll( );
  BOOST_CHECK_EQUAL( 1, first_int );

  // make sure things get sent only after flushing
  second_call_args second_args = { "Foo", 1 };
  // try with manual arg size discovery
  Grappa_call_on( 0, &second_call, &second_args, sizeof(second_args) );

  // try with null payload 
  Grappa_call_on( 0, &second_call, &second_args, sizeof(second_args), NULL, 0 );

  // nothing has been sent yet
  BOOST_CHECK_EQUAL( 0, second_int );

  // send
  a.flush( 0 );
  a.poll( );
  BOOST_CHECK_EQUAL( 2, second_int );

  // try with non-null payload 
  Grappa_call_on( 0, &second_call, &second_args, sizeof(second_args), &second_args, sizeof(second_args) );
  a.flush( 0 );
  a.poll( );
  BOOST_CHECK_EQUAL( 3, second_int );

  BOOST_MESSAGE( "make sure we flush when full" );
  int j = 0;
  size_t second_message_size = sizeof(second_args) + sizeof( AggregatorGenericCallHeader );
  for( int i = 0; i < global_aggregator.max_size() - second_message_size; i += second_message_size) {
    BOOST_MESSAGE( "sending " << second_args.i << " with sum " << j << " second_int " << second_int );
    //BOOST_CHECK_EQUAL( 3, second_int );
    //Grappa_call_on( 0, &second_call, &second_args );
    second_args.i = i;
    Grappa_call_on( 0, &second_call, &second_args, sizeof(second_args), NULL, 0 );
    BOOST_CHECK_EQUAL( 3, second_int ); 
    a.poll( );
    BOOST_CHECK_EQUAL( 3, second_int ); 
    j += i;
    BOOST_MESSAGE( "sent " << second_args.i << " with sum " << j << " second_int " << second_int );
  }
  BOOST_CHECK_EQUAL( 3, second_int ); 
  a.poll( );
  BOOST_CHECK_EQUAL( 3, second_int ); 
  second_args.i = 1;
  Grappa_call_on( 0, &second_call, &second_args, sizeof(second_args), NULL, 0 );
  a.poll( );
  BOOST_CHECK_EQUAL( j + 3, second_int );
  a.flush( 0 );
  a.poll( );
  BOOST_CHECK_EQUAL( j + 3 + 1, second_int );

  BOOST_MESSAGE("make sure the timer works");
  Grappa_call_on( 0, &first_call, &first_args);
  //Grappa_call_on( 0, &first_call, &first_args, NULL, 0 );
  BOOST_CHECK_EQUAL( 1, first_int );
  int64_t initial_ts, ts;
  Grappa::tick();
  for( initial_ts = ts = Grappa::timestamp(); ts - initial_ts < FLAGS_aggregator_autoflush_ticks - FLAGS_aggregator_autoflush_ticks/2; ) {
    a.poll();
    // watch out---the debug allocator slows things way down and makes this fail
#ifndef STL_DEBUG_ALLOCATOR
    BOOST_CHECK_EQUAL( 1, first_int );
#endif
    BOOST_MESSAGE( "initial " << initial_ts << " current " << ts );
    Grappa::tick();
    ts = Grappa::timestamp();
  }

  // watch out---the debug allocator slows things way down and makes this fail
#ifndef STL_DEBUG_ALLOCATOR
  BOOST_CHECK_EQUAL( 1, first_int );
#endif
  Grappa::tick();
  for( initial_ts = ts = Grappa::timestamp(); ts - initial_ts < FLAGS_aggregator_autoflush_ticks; ) {
    Grappa::tick();
    ts = Grappa::timestamp();
  }
  a.poll();
  BOOST_CHECK_EQUAL( 2, first_int );


  // make we flush in the correct order
  BOOST_CHECK_EQUAL( j + 3 + 1, second_int );
  BOOST_CHECK_EQUAL( a.remaining_size( 0 ), a.max_size() );
  BOOST_CHECK_EQUAL( a.remaining_size( 1 ), a.max_size() );

  // send to node 1
  second_args.i = 5;
  Grappa_call_on( 1, &second_call, &second_args, sizeof(second_args), NULL, 0 );
  BOOST_CHECK_EQUAL( j + 3 + 1, second_int );
  BOOST_CHECK_EQUAL( a.remaining_size( 1 ), a.max_size() - second_message_size );

  // send to node 0
  second_args.i = 1;
  Grappa_call_on( 0, &second_call, &second_args, sizeof(second_args), NULL, 0 );
  BOOST_CHECK_EQUAL( j + 3 + 1, second_int );
  BOOST_CHECK_EQUAL( a.remaining_size( 0 ), a.max_size() - second_message_size );
  BOOST_CHECK_EQUAL( a.remaining_size( 1 ), a.max_size() - second_message_size );

  // wait until just before timeout
  // for( int i = 0; i < FLAGS_aggregator_autoflush_ticks - 1; ++i ) {
  //   a.poll();
  // }
  Grappa::tick();
  for( initial_ts = ts = Grappa::timestamp(); ts - initial_ts < FLAGS_aggregator_autoflush_ticks - 100000; ) {
    BOOST_MESSAGE( "initial " << initial_ts << " current " << ts);
    // nothing has flushed yet
    BOOST_CHECK_EQUAL( j + 3 + 1, second_int );
    BOOST_CHECK_EQUAL( a.remaining_size( 0 ), a.max_size() - second_message_size );
    BOOST_CHECK_EQUAL( a.remaining_size( 1 ), a.max_size() - second_message_size );
    a.poll();
    Grappa::tick();
    ts = Grappa::timestamp();
  }

  BOOST_CHECK_EQUAL( a.remaining_size( 1 ), a.max_size() - second_message_size );
  // one more tick! node 1 flushes
  Grappa::tick();
  for( initial_ts = ts = Grappa::timestamp(); ts - initial_ts < FLAGS_aggregator_autoflush_ticks - 1000; ) {
    Grappa::tick();
    ts = Grappa::timestamp();
  }
  a.poll();
  BOOST_CHECK_EQUAL( j + 3 + 1, second_int );
  BOOST_CHECK_EQUAL( a.remaining_size( 0 ), a.max_size() - second_message_size );
  BOOST_CHECK_EQUAL( a.remaining_size( 1 ), a.max_size() );

  // one more tick! node 0 flushes
  a.poll();
  BOOST_CHECK_EQUAL( j + 3 + 2, second_int );
  BOOST_CHECK_EQUAL( a.remaining_size( 0 ), a.max_size() );
  BOOST_CHECK_EQUAL( a.remaining_size( 1 ), a.max_size() );

  } 
  // else {
  //   while( !done ) {
  //     a.poll();
  //   }
  // }
  s.barrier();
  a.finish();
  s.finish();

}

BOOST_AUTO_TEST_SUITE_END();
