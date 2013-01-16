
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
#include "Tasking.hpp"
#include "Addressing.hpp"
//#include "PerformanceTools.hpp"

#include "ConditionVariable.hpp"
#include "FullEmpty.hpp"
#include "Message.hpp"

namespace Grappa {

  /// Internal messaging functions
  namespace impl {

    /// @addtogroup Communication
    /// @{

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


      ///
      /// Interface to interact with messages
      ///

      /// Enqueue message to be sent
      inline void send( Grappa::impl::MessageBase * m ) {
	// get destination pointer
	Core * dest = &cores_[ m->destination_ ];
	Grappa::impl::MessageBase ** dest_ptr = &dest->messages_;

	// stitch in message
	do {
	  // read previous value
	  Grappa::impl::MessageBase * prev = *dest_ptr;
	  // append to current message
	  m->next_ = prev;
	  // insert current message
	} while( !__sync_bool_compare_and_swap( dest_ptr, prev, m ) );

	// now compute prefetch
	// TODO: update prefetch

	// acknowledge that we've sent message
	m->is_sent_ = true;
	Grappa::signal( m->cv_ );
      }


      ///
      /// methods for quick-and-dirty medium delivery process
      ///

      /// Active message to walk a buffer of received deserializers/functors and call them.
      static void deserialize_buffer_am( gasnet_token_t token, void * buf, size_t size ) {
        //DVLOG(5) << "Receiving buffer of size " << size << " through gasnet";
	Grappa::impl::MessageBase::deserialize_buffer( static_cast< char * >( buf ), size );
      }
      int deserialize_buffer_handle_;
      
      /// send a message that will be run in active message context. This requires very limited messages.
      void send_medium( Core * core ) {
        //DVLOG(5) << "Sending messages from " << core->messages_;

	// compute node from offset in array
	Node node = core - &cores_[0];

	size_t max_size = gasnet_AMMaxMedium();
	char buf[ max_size ] __attribute__ ((aligned (16)));
	char * end = Grappa::impl::MessageBase::serialize_to_buffer( buf, &(core->messages_), max_size );
	GASNET_CHECK( gasnet_AMRequestMedium0( node, deserialize_buffer_handle_, buf, end - buf ) );
        //DVLOG(5) << "Sent buffer of size " << end - buf << " through gasnet";
      }



      ///
      /// methods for RDMA-based delivery of messages
      ///

      /// Active message to deserialize/call the first entry of a buffer of received deserializers/functors
      static void deserialize_first_am( gasnet_token_t token, void * buf, size_t size ) {
        DVLOG(5) << "Receiving buffer of size " << size << " through gasnet; deserializing first entry";
	// lie about size so only the first entry is deserialized
	Grappa::impl::MessageBase::deserialize_buffer( static_cast< char * >( buf ), 1 );
      }
      int deserialize_first_handle_;
      
      /// Gasnet active message to receive a buffer
      static void reply_buffer_am( gasnet_token_t token, 
				   uint32_t gbw_hi, uint32_t gbw_lo, 
				   uint32_t buf_hi, uint32_t buf_lo, 
				   uint32_t bw_hi, uint32_t bw_lo ) {
	
      }
      int reply_buffer_handle_;

      void reply_with_buffer( GlobalAddress< int64_t > response_address, char * buf, BufferWaiter * bw ) {
	Node n = response_address.node();
	// deconstruct requesting bufferwaiter address
	uintptr_t gbw_uint = response_address.raw_bits();
	uint32_t gbw_hi = gbw_uint >> 32;
	uint32_t gbw_lo = gbw_uint & 0xffffffffL;

	// deconstruct buffer pointer
	uintptr_t buf_uint = reinterpret_cast< uintptr_t >( buf );
	uint32_t buf_hi = buf_uint >> 32;
	uint32_t buf_lo = buf_uint & 0xffffffffL;

	// deconstruct reply bufferwaiter address
	uintptr_t bw_uint = reinterpret_cast< uintptr_t >( bw );
	uint32_t bw_hi = bw_uint >> 32;
	uint32_t bw_lo = bw_uint & 0xffffffffL;

	GASNET_CHECK( gasnet_AMRequestShort6( node, reply_buffer_handle_, gbw_hi, gbw_lo, buf_hi, buf_lo, gbw_hi, gbw_lo ) );
      }

      /// task that is run to allocate space to receive a message
      static void deaggregation_task( int64_t size, GlobalAddress< int64_t > response_address ) {
	// allocate buffer for received messages
	char buf[ size ] __attribute__ ((aligned (16)));
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
  

      class Send_RDMA_Completion {
      public:
	void operator() {
	  
	}
      };


      void send_rdma( Core * core ) {
        DVLOG(5) << "Sending messages via RDMA from " << core->messages_;

	// compute node from offset in array
	Node node = core - &cores_[0];

	// allocate local buffer for send message
	size_t max_size = 1 << 13;
	char buf[ max_size + sizeof( Send_RDMA_Completion ) ];
	BufferWaiter bw( buf );

	// start a buffer moving in our direction
	request_buffer( node, max_size, &bw );

	// construct the message we will send
	char * end = Grappa::impl::MessageBase::serialize_to_buffer( buf, &(core->messages_), max_size );

	// wait until we have a buffer
	while( !bw.full() ) {
	  bw.wait();
	}
	
	// TODO: make asynchronous
	GASNET_CHECK( gasnet_AMRequestLong0( node, deserialize_buffer_handle_, buf, end - buf, dest ) );
        DVLOG(5) << "Sent buffer of size " << end - buf << " through gasnet";
      }



      Node mynode_;

    public:

      RDMAAggregator()
	: deserialize_buffer_handle_( -1 )
        , mynode_( -1 )
      {
      }

      // initialize and register with communicator
      void init() {
        cores_.resize( global_communicator.nodes() );
        mynode_ = global_communicator.mynode();
	request_buffer_handle_ = global_communicator.register_active_message_handler( &request_buffer_am );
	reply_buffer_handle_ = global_communicator.register_active_message_handler( &reply_buffer_am );
	deserialize_buffer_handle_ = global_communicator.register_active_message_handler( &deserialize_buffer_am );
	deserialize_first_handle_ = global_communicator.register_active_message_handler( &deserialize_first_am );
      }

      inline void enqueue( MessageBase * m ) {
	Core * c = &cores_[ m->destination_ ];
	/// TODO: CAS
	m->next_ = c->messages_;
	c->messages_ = m;
      }

      void flush( Node n ) {
	Core * c = &cores_[ n ];
	send_medium( c );
	//send_rdma( c );
      }

      // void poll( ) {
      //   MessageBase * received = &cores_[ mynode_ ];
      //   if( received ) {
      //     LOG(INFO) << "Receiving buffer through poll";
      //     Grappa::impl::MessageBase::deserialize_buffer( static_cast< char * >( received ) );
      //     received = NULL;
      //   }
      // }

    };

    /// @}
  };
};

#endif
