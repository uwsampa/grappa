
#include <gflags/gflags.h>

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
  char arg1[ 10 ];
};

int second_int = 0;

void second_call(second_call_args * args, size_t args_size, void * payload, size_t payload_size) 
{
  //BOOST_MESSAGE( "second_call: arg1=" << args->arg1 );
  second_int++;
  BOOST_CHECK( payload_size == 0 || payload_size == args_size );
}


BOOST_AUTO_TEST_CASE( test1 ) {
  Communicator s;
  s.init( &(boost::unit_test::framework::master_test_suite().argc),
          &(boost::unit_test::framework::master_test_suite().argv) );

  Aggregator a( &s );

  s.activate();
  if( s.mynode() == 0 ) {

  // make sure we can send something
  first_call_args first_args = { 1, 2.3 };

  // try with automagic arg size discovery
  SoftXMT_call_on( 0, &first_call, &first_args );

  a.flush( 0 );
  BOOST_CHECK_EQUAL( 1, first_int );

  // make sure things get sent only after flushing
  second_call_args second_args = { "Foo" };
  // try with manual arg size discovery
  SoftXMT_call_on( 0, &second_call, &second_args, sizeof(second_args) );

  // try with null payload 
  SoftXMT_call_on( 0, &second_call, &second_args, sizeof(second_args), NULL, 0 );

  // nothing has been sent yet
  BOOST_CHECK_EQUAL( 0, second_int );

  // send
  a.flush( 0 );
  BOOST_CHECK_EQUAL( 2, second_int );

  // try with non-null payload 
  SoftXMT_call_on( 0, &second_call, &second_args, sizeof(second_args), &second_args, sizeof(second_args) );
  a.flush( 0 );
  BOOST_CHECK_EQUAL( 3, second_int );

  // make sure we flush when full
  int j = 0;
  for( int i = 0; i < 4024; i += sizeof(second_args) + sizeof( AggregatorGenericCallHeader ) ) {
    //BOOST_CHECK_EQUAL( 3, second_int );
    //SoftXMT_call_on( 0, &second_call, &second_args );
    SoftXMT_call_on( 0, &second_call, &second_args, sizeof(second_args), NULL, 0 );
    ++j;
  }
  BOOST_CHECK_EQUAL( 3 + j - 1, second_int );
  a.flush( 0 );
  BOOST_CHECK_EQUAL( 3 + j, second_int );

  // make sure the timer works
  SoftXMT_call_on( 0, &first_call, &first_args);
  //SoftXMT_call_on( 0, &first_call, &first_args, NULL, 0 );
  BOOST_CHECK_EQUAL( 1, first_int );
  int64_t ts = a.get_timestamp();
  for( int i = 0; i < FLAGS_aggregator_autoflush_ticks + 1; ++i ) {
    a.poll();
    //BOOST_CHECK_EQUAL( ++ts, a.get_timestamp() );
  }
//   sleep(10);
//   a.poll();
  BOOST_CHECK_EQUAL( 2, first_int );

  }
  s.finish();

}

BOOST_AUTO_TEST_SUITE_END();
