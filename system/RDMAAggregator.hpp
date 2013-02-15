
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

#include "MessageBase.hpp"
#include "ConditionVariableLocal.hpp"
#include "FullEmpty.hpp"

#include "Statistics.hpp"

DECLARE_int64( target_size );
DECLARE_int64( flush_interval );

/// stats for application messages
GRAPPA_DECLARE_STAT( SimpleStatistic<int64_t>, app_messages_enqueue );
GRAPPA_DECLARE_STAT( SimpleStatistic<int64_t>, app_messages_enqueue_cas );
GRAPPA_DECLARE_STAT( SimpleStatistic<int64_t>, app_messages_immediate );

/// stats for aggregated messages
GRAPPA_DECLARE_STAT( SummarizingStatistic<int64_t>, rdma_message_bytes );

/// stats for RDMA Aggregator events
GRAPPA_DECLARE_STAT( SimpleStatistic<int64_t>, rdma_receive_start );
GRAPPA_DECLARE_STAT( SimpleStatistic<int64_t>, rdma_receive_end );
GRAPPA_DECLARE_STAT( SimpleStatistic<int64_t>, rdma_send_start );
GRAPPA_DECLARE_STAT( SimpleStatistic<int64_t>, rdma_send_end );
GRAPPA_DECLARE_STAT( SimpleStatistic<int64_t>, rdma_capacity_flushes );
GRAPPA_DECLARE_STAT( SimpleStatistic<int64_t>, rdma_timeout_flushes );
GRAPPA_DECLARE_STAT( SimpleStatistic<int64_t>, rdma_requested_flushes );


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

    static const int prefetch_dist = 5;
    static const int prefetch_type = 0; // 0 (non-temporal) or 3 (L1) are probably the best choice

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
      //private:
    public:
      
      Core mycore_;
      Node mynode_;
      Core cores_per_node_;
      Core total_cores_;

      bool flushing_;

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
      char * aggregate_to_buffer( char * buffer, Grappa::impl::MessageBase ** message_ptr, size_t max = -1, size_t * count = NULL );
      
      // Deserialize and call a buffer of messages
      static char * deaggregate_buffer( char * buffer, size_t size );



      /// Sender size of RDMA transmission.
      void send_rdma( Core core, Grappa::impl::MessageList ml );
      void send_medium( Core core, Grappa::impl::MessageList ml );

      /// task that is run to allocate space to receive a message      
      static void deaggregation_task( GlobalAddress< FullEmpty < ReceiveBufferInfo > > callback_ptr );

    public:

      RDMAAggregator()
        : mycore_( -1 )
        , mynode_( -1 )
        , cores_per_node_( -1 )
        , total_cores_( -1 )
        , flushing_( false )
        , deserialize_buffer_handle_( -1 )
        , deserialize_first_handle_( -1 )
      {
      }

      // initialize and register with communicator
      void init() {
        cores_.resize( global_communicator.nodes() );
        mycore_ = global_communicator.mynode();
        total_cores_ = global_communicator.nodes();
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
        app_messages_enqueue++;
        DVLOG(4) << "Enqueued message " << m << ": " << m->typestr();

        // get destination pointer
        Core core = m->destination_;
        CoreData * dest = &cores_[ core ];
        //Grappa::impl::MessageBase ** dest_ptr = &dest->messages_;
        Grappa::impl::MessageList * dest_ptr = &(dest->messages_);
        Grappa::impl::MessageList old_ml, new_ml, swap_ml;

        // new values computed from previous totals
        int count = 0;
        size_t size = 0;
        swap_ml.raw_ = 0;

        bool spawn_send = false;

        // prepare to stitch in message
        set_pointer( &new_ml, m );

        // stitch in message
        do {
          // read previous value
          old_ml = *dest_ptr;

          // add previous count/estimated size
          count = 1 + old_ml.count_;
          size = m->serialized_size();
          if( count > 1 ) {
            size += dest->prefetch_queue_[ ( old_ml.count_ ) % prefetch_dist ].size_;
          }

          new_ml.count_ = count; 

          // append previous list to current message
          m->next_ = get_pointer( &old_ml );
          // set prefetch to the oldest pointer we remember
          // index prefetch queue by count since we haven't overwritten it in yet.
          m->prefetch_ = get_pointer( &(dest->prefetch_queue_[ count % prefetch_dist ]) );

          spawn_send = size > FLAGS_target_size;

          // if it looks like we should send
          if( spawn_send ) {
            swap_ml.raw_ = 0; // leave the list empty
          } else {
            // stitch in this message
            swap_ml = new_ml;
          }

          // now try to insert current message (and count attempt)
          app_messages_enqueue_cas++;
        } while( !__sync_bool_compare_and_swap( &(dest_ptr->raw_), old_ml.raw_, swap_ml.raw_ ) );

        CHECK_NE( m, get_pointer( &old_ml ) ) << "Message already enqueued!";
        //dump_ml( new_ml );

        if( !spawn_send ) {
          // now fill in prefetch pointer and size
          dest->prefetch_queue_[ count % prefetch_dist ].size_ = size;
          set_pointer( &(dest->prefetch_queue_[ count % prefetch_dist ]), m );
        } else {
          rdma_capacity_flushes++;
          //Grappa::ConditionVariable cv;
          //Grappa::privateTask( [core,new_ml,&cv] {
          Grappa::privateTask( [core,new_ml] {
              global_rdma_aggregator.send_rdma( core, new_ml );
              //global_rdma_aggregator.send_medium( core, new_ml );
              //Grappa::signal( &cv );
            });
          //Grappa::wait( &cv );
        }
      }


      /// send a message that will be run in active message context. This requires very limited messages.
      void send_immediate( Grappa::impl::MessageBase * m ) {
        app_messages_immediate++;

        // create temporary buffer
        const size_t size = m->serialized_size();
        char buf[ size ] __attribute__ ((aligned (16)));

        // serialize to buffer
        Grappa::impl::MessageBase * tmp = m;
        while( tmp != nullptr ) {
          DVLOG(5) << "Serializing message from " << tmp;
          char * end = aggregate_to_buffer( &buf[0], &tmp, size );
          DVLOG(5) << "After serializing, pointer was " << tmp;
          DCHECK_EQ( end - buf, size );
          
          // send
          GASNET_CHECK( gasnet_AMRequestMedium0( m->destination_, deserialize_buffer_handle_, buf, size ) );
        }
      }

      void dump_ml( Grappa::impl::MessageList ml ) {
        return;
        Grappa::impl::MessageBase * m = get_pointer( &ml );
        while( m ) {
          LOG(INFO) << "Grabbed message " << m;
          m = m->next_;
        }
      }

      Grappa::impl::MessageList grab_messages( Core c ) {
        Grappa::impl::MessageList * dest_ptr = &cores_[ c ].messages_;
        Grappa::impl::MessageList old_ml, new_ml;

        do {
          // read previous value
          old_ml = *dest_ptr;
          new_ml.raw_ = 0;
          // insert current message
        } while( !__sync_bool_compare_and_swap( &(dest_ptr->raw_), old_ml.raw_, new_ml.raw_ ) );
        //dump_ml( old_ml );
        return old_ml;
      }

      void flush( Core c ) {
        rdma_requested_flushes++;

        Grappa::impl::MessageList ml = grab_messages( c );
        if( ml.raw_ != 0 ) {
          Grappa::ConditionVariable cv;
          // run on its own task so it has a full stack
          Grappa::privateTask( [&cv, c, ml] {
              global_rdma_aggregator.send_rdma( c, ml );
              //global_rdma_aggregator.send_medium( c, ml );
              Grappa::signal( &cv );
            } );
          Grappa::wait( &cv );
        } 
      }

      void idle_flush() {
        static Grappa_Timestamp last = 0;
        Grappa_Timestamp current = Grappa_get_timestamp();
        if( (current - last) > FLAGS_flush_interval ) {
          flushing_ = true;
          last = current;
          rdma_timeout_flushes++;
          for( int i = 0; i < total_cores_; ++i ) {
            Grappa::impl::MessageList ml = grab_messages( i );
            if( ml.raw_ != 0 ) {
              // run on its own task so it has a full stack
              Grappa::privateTask( [ i, ml ] {
                  global_rdma_aggregator.send_rdma( i, ml );
                  //global_rdma_aggregator.send_medium( i, ml );
                });
            }
          }
        }
      }

    };

    /// @}
  }
}

#endif
