
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
#include "MessageBase.hpp"

namespace Grappa {

  typedef Node Core;

  /// Internal messaging functions
  namespace impl {

    /// @addtogroup Communication
    /// @{

    class MessageBase;

    /// global aggregator instance
    class RDMAAggregator;
    extern Grappa::impl::RDMAAggregator global_rdma_aggregator;


    struct MessageList {
      union {
        struct {
          unsigned count_ : 16;
          intptr_t pointer_ : 48;
        };
        intptr_t raw_; // unnecessary; just to ensure alignment
      };
    };
    
    struct PrefetchEntry {
      union {
        struct {
          unsigned size_ : 16;
          intptr_t pointer_ : 48;
        };
        intptr_t raw_; // unnecessary; just to ensure alignment
      };
    };

    /// used to start deaggregation of remote message
    struct SendBufferInfo {
      int8_t offset;
      int32_t actual_size;
    };

    /// refers to a buffer on the remote machine
    struct ReceiveBufferInfo {
      char * buffer;
      FullEmpty< SendBufferInfo > * info_ptr;
    };



    /// accesor for pointer in message lists
    template< typename MessageListOrPrefetchEntry >
    static inline Grappa::impl::MessageBase * get_pointer( MessageListOrPrefetchEntry * ml ) { 
      return reinterpret_cast< Grappa::impl::MessageBase * >( ml->pointer_ );
    }

    /// accesor for wait list in synchronization object
    template< typename MessageListOrPrefetchEntry >
    static inline void set_pointer( MessageListOrPrefetchEntry * ml, Grappa::impl::MessageBase * m ) { 
      ml->pointer_ = reinterpret_cast< intptr_t >( m ); 
    }

    static const int prefetch_dist = 4;

    struct CoreData {
      // things that must be included per core:
      //
      // pointer to message list
      // size of message list
      // index into prefetch queue
      //
      // prefetch queue
      //
      // pointer to list of shared destinations for cross-core aggregation
      //
      // pointer to list of messages to receive
      // 
      
      // should we use shared memory?
      
      Grappa::impl::MessageList messages_;
      Grappa::impl::PrefetchEntry prefetch_queue_[ prefetch_dist ];

      // lowest-numbered core that is on a node
      Core representative_core_;

      Grappa::FullEmpty< ReceiveBufferInfo > remote_buffer_info_;

      Grappa::impl::MessageBase * received_messages_;

      
      CoreData() 
        : messages_()
        , prefetch_queue_() 
        , remote_buffer_info_()
        , received_messages_(NULL)
      { }
    } __attribute__ ((aligned(64)));


    /// New aggregator design
    class RDMAAggregator {
    private:
      
      Core mycore_;
      Node mynode_;
      Core cores_per_node_;

      /// actual aggregation buffers
      /// TODO: one process allocate on shared pages
      /// assume colocated cores are stored next to each other

      std::vector< CoreData > cores_;


      /// Active message to walk a buffer of received deserializers/functors and call them.
      static void deserialize_buffer_am( gasnet_token_t token, void * buf, size_t size );
      int deserialize_buffer_handle_;
      
      /// Active message to deserialize/call the first entry of a buffer of received deserializers/functors
      static void deserialize_first_am( gasnet_token_t token, void * buf, size_t size );
      int deserialize_first_handle_;
      
      /// Chase a list of messages and serialize them into a buffer.
      /// Modifies pointer to list to support size-limited-ish aggregation
      /// TODO: make this a hard limit?
      char * aggregate_to_buffer( char * buffer, Grappa::impl::MessageBase ** message_ptr, size_t max = -1 );
      
      // Deserialize and call a buffer of messages
      static char * deaggregate_buffer( char * buffer, size_t size );


      /// Sender size of RDMA transmission.
      void send_rdma( Core core );

      /// task that is run to allocate space to receive a message      
      static void deaggregation_task( GlobalAddress< FullEmpty < ReceiveBufferInfo > > callback_ptr );


    public:

      RDMAAggregator()
        : mycore_( -1 )
        , mynode_( -1 )
        , cores_per_node_( -1 )
        , deserialize_buffer_handle_( -1 )
        , deserialize_first_handle_( -1 )
      {
      }

      // initialize and register with communicator
      void init() {
        cores_.resize( global_communicator.nodes() );
        mycore_ = global_communicator.mynode();
        deserialize_buffer_handle_ = global_communicator.register_active_message_handler( &deserialize_buffer_am );
        deserialize_first_handle_ = global_communicator.register_active_message_handler( &deserialize_first_am );
      }

      // void poll( ) {
      //   // there are two places 
      //   MessageBase * received = &cores_[ mynode_ ];
      //   if( received ) {
      //     LOG(INFO) << "Receiving buffer through poll";
      //     Grappa::impl::MessageBase::deserialize_buffer( static_cast< char * >( received ) );
      //     received = NULL;
      //   }
      // }

      /// Enqueue message to be sent
      inline void enqueue( Grappa::impl::MessageBase * m ) {
        // get destination pointer
        CoreData * dest = &cores_[ m->destination_ ];
        //Grappa::impl::MessageBase ** dest_ptr = &dest->messages_;
        Grappa::impl::MessageList * dest_ptr = &(dest->messages_);
        Grappa::impl::MessageList old_ml, new_ml;
        int count;

        // stitch in message
        do {
          // set pointer in new value to current message
          set_pointer( &new_ml, m );

          // read previous value
          old_ml = *dest_ptr;

          // count this message
          new_ml.count_ = count = old_ml.count_ + 1;

          // insert current message
        } while( !__sync_bool_compare_and_swap( &(dest_ptr->raw_), old_ml.raw_, new_ml.raw_ ) );

        // append previous list to current message
        m->next_ = get_pointer( &old_ml );
        m->prefetch_ = get_pointer( &(dest->prefetch_queue_[ (count + 1 ) % prefetch_dist ]) );

        // now compute prefetch
        PrefetchEntry new_pe;
        set_pointer( &(dest->prefetch_queue_[ count % prefetch_dist ]), m );
        dest->prefetch_queue_[ count % prefetch_dist ].size_ = dest->prefetch_queue_[ ( count - 1 ) % prefetch_dist ].size_ + m->size();
      }


      /// send a message that will be run in active message context. This requires very limited messages.
      void send_immediate( Grappa::impl::MessageBase * m ) {
        // create temporary buffer
        const size_t size = m->size();
        char buf[ size ] __attribute__ ((aligned (16)));

        // serialize to buffer
        Grappa::impl::MessageBase * tmp = m;
        DVLOG(5) << "Serializing message from " << tmp;
        char * end = aggregate_to_buffer( &buf[0], &tmp, size );
        DVLOG(5) << "After serializing, pointer was " << tmp;
        DCHECK_EQ( end - buf, size );

        // send
        GASNET_CHECK( gasnet_AMRequestMedium0( m->destination_, deserialize_buffer_handle_, buf, size ) );
      }

      void flush( Core c ) {
        LOG(INFO) << "Flushing";
        send_rdma( c );
        LOG(INFO) << "Flush complete.";
      }

    };

    /// @}
  };
};

#endif
