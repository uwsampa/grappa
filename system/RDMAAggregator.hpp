
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

#include "ConditionVariable.hpp"
#include "Message.hpp"

namespace Grappa {

  /// Internal messaging functions
  namespace impl {

    /// @addtogroup Communication
    /// @{


    class BufferWaiter : public Grappa::ConditionVariable {
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
    };








    /// global aggregator instance
    class RDMAAggregator;
    extern Grappa::impl::RDMAAggregator global_rdma_aggregator;


    /// New aggregator design
    class RDMAAggregator {
    private:
      
      struct Core {
	MessageBase * messages_;
	size_t size_;
	Core * next_;
	MessageBase * received_messages_;
	Core() : messages_(NULL), size_(0), next_(NULL), received_messages_(NULL) { }
      };
      std::vector< Core > cores_;

      /// Active message to walk a buffer of received deserializers/functors and call them.
      static void deserialize_buffer_am( gasnet_token_t token, void * buf, size_t size ) {
	Grappa::impl::MessageBase::deserialize_buffer( static_cast< char * >( buf ), size );
      }
      int deserialize_buffer_handle_;
      
      /// send a message that will be run in active message context. This requires very limited messages.
      void send_medium( Core * core ) {
	// compute node from offset in array
	Node node = core - &cores_[0];

	size_t max_size = gasnet_AMMaxMedium();
	char buf[ max_size ];
	char * end = Grappa::impl::MessageBase::serialize_to_buffer( buf, &(core->messages_) );
	GASNET_CHECK( gasnet_AMRequestMedium0( node, deserialize_buffer_handle_, buf, end - buf ) );
      }

      // /// send a message
      // void send_rdma( MessageBase * ) {
      // 	// compute node from offset in array
      // 	Node node = &core - &cores_[0];

	
      // }


      /// Gasnet active message to receive a buffer
      static void reply_buffer_am( gasnet_token_t token, uint32_t size, uint32_t hi_address, uint32_t lo_address ) {
	
      }
      int reply_buffer_handle_;

      void reply_with_buffer( GlobalAddress< int64_t > response_address, char * buf, BufferWaiter * bw ) {
	
      }




      /// task that is run to allocate space to receive a message
      static void deaggregation_task( int64_t size, GlobalAddress< int64_t > response_address ) {
	// allocate buffer for received messages
	char buf[ size ];
	BufferWaiter bw( buf );
	
	// send pointer to buffer 
	Grappa::impl::global_rdma_aggregator.reply_with_buffer( response_address, &buf[0], &bw );

	// wait for buffer to be filled
	while( !bw.full() ) {
	  bw.wait();
	}
	
	// deaggregate and execute
	MessageBase::deserialize_buffer( buf, size );

	// done
      }

      /// Gasnet active message to request a buffer
      static void request_buffer_am( gasnet_token_t token, uint32_t size, uint32_t hi_address, uint32_t lo_address ) {
	uintptr_t response_address = hi_address;
	response_address = (response_address << 32) | lo_address;
	GlobalAddress< int64_t > gbw = GlobalAddress< int64_t >::Raw( response_address );

	// spawn
	Grappa_privateTask( &deaggregation_task, static_cast< int64_t >( size ), gbw );
      }
      int request_buffer_handle_;

      /// Request a buffer
      void request_buffer( Node node, uint32_t size, BufferWaiter * bw ) {
	GlobalAddress< BufferWaiter > gbw = make_global( bw );
	uintptr_t gbw_uint = gbw.raw_bits();
	uint32_t gbw_hi = gbw_uint >> 32;
	uint32_t gbw_lo = gbw_uint & 0xffffffffL;
	GASNET_CHECK( gasnet_AMRequestShort3( node, request_buffer_handle_, size, gbw_hi, gbw_lo ) );
      }
  

    public:

      RDMAAggregator()
	: deserialize_buffer_handle_( -1 )
      {}

      // initialize and register with communicator
      void init() {
	request_buffer_handle_ = global_communicator.register_active_message_handler( &request_buffer_am );
	reply_buffer_handle_ = global_communicator.register_active_message_handler( &reply_buffer_am );
	deserialize_buffer_handle_ = global_communicator.register_active_message_handler( &deserialize_buffer_am );
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

    };

    /// @}
  };
};

#endif
