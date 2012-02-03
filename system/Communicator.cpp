
#include <cassert>

#include "Communicator.hpp"

Communicator * global_communicator;

///construct
Communicator::Communicator( )
  : handlers_()
  , registration_is_allowed_( false )
  , communication_is_allowed_( false )
{ 
  assert( global_communicator == NULL );
  global_communicator = this;
}


/// initialize communication layer. After this call, node parameters
/// may be queried and handlers may be registered, but no
/// communication is allowed.
void Communicator::init( int * argc_p, char ** argv_p[] ) {
  GASNET_CHECK( gasnet_init( argc_p, argv_p ) ); 
  // make sure the Node type is big enough for our system
  assert( static_cast< int64_t >( gasnet_nodes() ) <= (1L << sizeof(Node) * 8) && 
          "Node type is too small for number of nodes in job" );
  registration_is_allowed_ = true;
}

/// activate communication layer. finishes registering handlers and
/// any shared memory segment. After this call, network communication
/// is allowed, but no more handler registrations are allowed.
void Communicator::activate() {
    assert( registration_is_allowed_ );
  GASNET_CHECK( gasnet_attach( &handlers_[0], handlers_.size(), // install active message handlers
                               0,  // no shared segment right now
                               0 ) );
  registration_is_allowed_ = false;
  communication_is_allowed_ = true;
}

/// tear down communication layer.
void Communicator::finish(int retval) {
  communication_is_allowed_ = false;
  //gasnet_exit( retval );
}


/*
 * tests
 */
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE( Communicator_tests );

bool success = false;
void foo( gasnet_token_t token ){
  success = true;
}


void bar( gasnet_token_t token, char * buf, size_t len ){
  int64_t sum = 0;
  for( int i = 0; i < len; ++i ) {
    //printf("%d: %d\n", i, buf[ i ] );
    sum += buf[ i ];
  }
  BOOST_CHECK_EQUAL( len, gasnet_AMMaxMedium() );
  BOOST_CHECK_EQUAL( sum, len );
  success = (sum == len);
}

BOOST_AUTO_TEST_CASE( test1 ) {
  Communicator s;
  s.init( &(boost::unit_test::framework::master_test_suite().argc),
          &(boost::unit_test::framework::master_test_suite().argv) );
  
  //  int foo_h = s.register_active_message( reinterpret_cast< HandlerPointer >( &foo ) );
  int foo_h = s.register_active_message_handler( &foo );
  int bar_h = s.register_active_message_handler( &bar );
  s.activate();

  // make sure we've registered the handler properly and gasnet can call it
  BOOST_CHECK_EQUAL( success, false );
  gasnet_AMRequestShort0( s.mynode(), foo_h );
  BOOST_CHECK_EQUAL( success, true );

    // make sure we've registered the handler properly and we can call it
  success = false;
  s.send( s.mynode(), foo_h, NULL, 0 );
  BOOST_CHECK_EQUAL( success, true );


  // make sure we can pass data to a handler
  success = false;
  const size_t bardata_size = gasnet_AMMaxMedium();
  char bardata[ bardata_size ];
  memset( &bardata[0], 1, bardata_size );
  s.send( s.mynode(), bar_h, &bardata[0], bardata_size );
  BOOST_CHECK_EQUAL( success, true );

  s.finish();

}

BOOST_AUTO_TEST_SUITE_END();

