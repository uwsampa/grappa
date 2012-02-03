
#ifndef __COMMUNICATOR_HPP__
#define __COMMUNICATOR_HPP__

/// Definition of SoftXMT communication layer wrapper class.  This
/// class' functionality is implemented using the GASNet library.
///
/// There are three phases in this class' lifetime. 
///
/// -# after the constructor call, before the init call. No method
/// calls are allowed at this point.
///
/// -# after the init() call. Handlers can be registered, and node
/// parameters can be queried. Communication is not allowed at this
/// point.
///
/// -# after the activate() call. Communication is allowed, and node
/// parameters may be queried. No more handlers may be
/// registered. Applications spend most of their time in this phase.

#include <cassert>
#include <vector>
#include "common.hpp"

#include <gasnet.h>
#include "gasnet_helpers.h"

/// Common pointer type for active message handlers used by GASNet
/// library. Actual handlers may have arguments.
typedef void (*HandlerPointer)();    

/// Type for Node ID. 
typedef int16_t Node;

/// Maximum size of GASNet medium active message payload. 
///
/// Each GASNet medium message carries a bunch of overhead:
///  - a 4-byte immediate value in the infiniband message that t impact the MTU
///  - 4 bytes of flow control
///  - 4 bytes of payload size
///  - space for up to 16 4-byte arguments, which may be reused for
///    payload when not used for arguments
///
/// GASNETC_MAX_MEDIUM is the maximum payload size allowed for GASNet
/// medium active messages with up to 16 4-byte argments. For
/// Infiniband this will be nearly 4KB, Infiniband's maximum MTU. This
/// is true even if the network is configured with a smaller MTU.
//#define GASNET_NOARG_MAX_MEDIUM (GASNETC_MAX_MEDIUM + sizeof( int32_t ) * 16)


/// Communication layer wrapper class.
class Communicator {
private:
  DISALLOW_COPY_AND_ASSIGN( Communicator );

  /// GASNet allows only a limited number of user-defined active message handlers. This is the first handler ID allowed.
  static const int initial_handler_index = 128;
  /// GASNet allows only a limited number of user-defined active message handlers. This is the last handler ID allowed.
  static const int maximum_handler_index = 255;

  /// used as intermediate storage for active message handlers during the registration process.
  std::vector< gasnet_handlerentry_t > handlers_;

  /// Are we in the phase that allows handler registration?
  bool registration_is_allowed_;

  /// Are we in the phase that allows communication?
  bool communication_is_allowed_;

public:

  /// Maximum size of GASNet medium active message payload. 
  ///
  /// Each GASNet medium message carries a bunch of overhead:
  ///  - a 4-byte immediate value in the infiniband message that t impact the MTU
  ///  - 4 bytes of flow control
  ///  - 4 bytes of payload size
  ///  - space for some number of 4-byte arguments, which may be
  ///    reused for payload when not used for arguments
  ///
  /// gasnet_AMMaxMedium() is the maximum payload size allowed for
  /// GASNet medium active messages with up to gasnet_AMMaxArgs()
  /// 4-byte argments. For Infiniband this will be nearly 4KB,
  /// Infiniband's maximum MTU. This is true even if the network is
  /// configured with a smaller MTU, so if your goal is to minimize
  /// network overhead and your network is configured with a smaller
  /// MTU, it may be best to send messages smaller than this.
  static const int maximum_message_payload_size = 
    gasnet_AMMaxMedium() +                  // worst-case maximum, plus
    sizeof( int32_t ) * gasnet_AMMaxArgs(); // the space that would be used for arguments

  /// construct
  Communicator( );

  void init( int * argc_p, char ** argv_p[] );

  /// Register an active message handler with the communication
  /// layer. This is a template method so the user doesn't have to
  /// cast the pointer to the handler pointer type.
  ///
  ///   @param hp Pointer to handler function. Method pointers are not supported.
  ///   @return Handler ID to be used in active message call.
  template< typename HP >
  inline int register_active_message_handler( HP hp ) {
    assert( registration_is_allowed_ );
    int current_handler_index = initial_handler_index + handlers_.size();
    assert( current_handler_index < maximum_handler_index );
    gasnet_handlerentry_t handler = { current_handler_index, 
                                      reinterpret_cast< HandlerPointer >( hp ) };
    handlers_.push_back( handler );
    return current_handler_index;
  }

  void activate();
  void finish( int retval = 0);

  /// Get id of this node
  inline Node mynode() const { 
    assert( registration_is_allowed_ || communication_is_allowed_ );
    return gasnet_mynode(); 
  }

  /// Get total count of nodes
  inline Node nodes() const { 
    assert( registration_is_allowed_ || communication_is_allowed_ );
    return gasnet_nodes(); 
  }

  /// Global (anonymous) barrier
  inline void barrier() {
    assert( communication_is_allowed_ );
    gasnet_barrier_notify( 0, GASNET_BARRIERFLAG_ANONYMOUS );
    GASNET_CHECK( gasnet_barrier_wait( 0, GASNET_BARRIERFLAG_ANONYMOUS ) );
  }

  /// Send no-argment active message with payload
  inline void send( Node node, int handler, void * buf, size_t size ) { 
    assert( communication_is_allowed_ );
    assert( size < maximum_message_payload_size ); // make sure payload isn't too big
    GASNET_CHECK( gasnet_AMRequestMedium0( node, handler, buf, size ) );
  }

  /// poll messaging layer
  inline void poll() { 
    assert( communication_is_allowed_ );
    GASNET_CHECK( gasnet_AMPoll() ); 
  }
      
};

extern Communicator * global_communicator;

#endif
