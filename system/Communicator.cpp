
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
  // TODO: for now, don't call gasnet exit. should we in future?
  //gasnet_exit( retval );
}


