
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.


#ifndef __RDMAAGGREGATOR_HPP__
#define __RDMAAGGREGATOR_HPP__

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "Communicator.hpp"
#include "tasks/Thread.hpp"
#include "tasks/TaskingScheduler.hpp"
//#include "PerformanceTools.hpp"

#include "Message.hpp"

namespace Grappa {

  /// Internal messaging functions
  namespace impl {

    /// @addtogroup Communication
    /// @{


    class BufferWaiter : public Synchronization::ConditionVariable {
    private:
      char * address_;
      bool filled_;
    public:
      BufferWaiter( char * address ) 
	: ConditionVariable()
	, address_( address )
	, filled_( false )
      { }

      inline bool full() { return filled_; }
      
      void wait() {
	CHECK( !filled_ ) << "Tried to wait while buffer was already filled";
	ConditionVariable::wait();
      }

      void signal() {
	CHECK( !filled_ ) << "Tried to signal while buffer was already filled";
	filled_ = true;
	ConditionVariable::signal();
      }
    }

    /// New aggregator design
    class RDMAAggregator {
    private:

      struct Core {
	MessageBase * messages_;
	size_t size_;
	Core * next_;
	MessageBase * received_messages_
	Core() : messages_(nullptr), size_(0), next_(nullptr), received_messages_(nullptr) { }
      };
      std::vector< Core > cores_;

      /// send a message that will be run in active message context. This requires very limited messages.
      void send_medium( Core * core ) {
	// compute node from offset in array
	Node node = &core - &cores_[0];

	size_t max_size = gasnet_AMMaxMedium();
	char buf[ max_size ];
	char * end = Grappa::impl::MessageBase::serialize_to_buffer( buf, &(core->messages_) );
	GASNET_CHECK( gasnet_AMRequestMedium0( node, deserialize_buffer_handle_, buf, end - buf ) );
      }

      void enqueue( MessageBase * m ) {
	Core * c = &cores_[ m->destination_ ];
	m->next_ = c->messages_;
	c->messages_ = m;
      }

      void flush( Node n ) {
	Core * c = &cores_[ n ];
	send_medium( c );
      }

      // /// send a message
      // void send_rdma( MessageBase * ) {
      // 	// compute node from offset in array
      // 	Node node = &core - &cores_[0];

	
      // }

      /// task that is run to allocate space to receive a message
      static void deaggregation_task( int64_t size, GlobalAddress< int64_t > response_address, void * unused ) {
	// allocate buffer for received messages
	char buf[ size ] = { 0 };
	BufferWaiter( &buf ) bw;
	
	// send pointer to buffer 
	reply_with_buffer( response_address, &buf, &bw );

	// wait for buffer to be filled
	while( !bw.full() ) {
	  bw.wait();
	}
	
	// deaggregate and execute
	MessageBase::deserialize_buffer( buf, size );

	// done
      }

      /// Gasnet active message to receive a buffer
      static void buffer_response_am( gasnet_token_t token, uint32_t size, uint32_t hi_address, uint32_t lo_address ) {
	
      }
      int buffer_response_handle_;

      /// Gasnet active message to request a buffer
      static void buffer_request_am( gasnet_token_t token, uint32_t size, uint32_t hi_address, uint32_t lo_address ) {
	uintptr_t response_address = (hi_address << 32) | lo_address;
	// spawn
	Grappa_privateTask( &deaggregation_task, size, response_address, NULL );
      }
      int buffer_request_handle_;

      /// Request a buffer
      void request_buffer( Node node, uint32_t size, BufferWaiter * bw ) {
	GlobalAddress< BufferWaiter > gbw = make_global( bw );
	uint32_t gbw_hi = gbw >> 32;
	uint32_t gbw_lo = gbw & 0xffffffffL;
	GASNET_CHECK( gasnet_AMRequestShort3( node, buffer_request_handle_, size, gbw_hi, gbw_lo ) );
      }
  

public:

  RDMAAggregator()
  {}

  // initialize and register with communicator
  init() {
    request_buffer_handle_ = global_communicator.register_active_message_handler( &request_buffer );
    reply_buffer_handle_ = global_communicator.register_active_message_handler( &reply_buffer );
  }

};

#endif
