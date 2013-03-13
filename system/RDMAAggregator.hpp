
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

#include "LocaleSharedMemory.hpp"

#include "MessageBase.hpp"
#include "RDMABuffer.hpp"

#include "ConditionVariableLocal.hpp"
#include "CountingSemaphoreLocal.hpp"
#include "FullEmpty.hpp"

#include "ReusePool.hpp"
#include "ReuseList.hpp"

#include "Statistics.hpp"

// #include <boost/interprocess/containers/vector.hpp>

DECLARE_int64( target_size );

/// stats for application messages
GRAPPA_DECLARE_STAT( SimpleStatistic<int64_t>, app_messages_enqueue );
GRAPPA_DECLARE_STAT( SimpleStatistic<int64_t>, app_messages_enqueue_cas );
GRAPPA_DECLARE_STAT( SimpleStatistic<int64_t>, app_messages_immediate );

/// stats for RDMA Aggregator events
GRAPPA_DECLARE_STAT( SimpleStatistic<int64_t>, rdma_capacity_flushes );
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

    static const int prefetch_dist = 4;
    static const int prefetch_type = 0; // 0 (non-temporal) or 3 (L1) are probably the best choice
    static const int remote_buffer_pool_size = 6;

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


      /// all this should be on one cache line
      Grappa::impl::MessageList messages_;
      Grappa::impl::PrefetchEntry prefetch_queue_[ prefetch_dist ];

      // lowest-numbered core that is on a node
      Core representative_core_;

      int64_t pad[2]; 

      ///
      /// another cache line
      ///

      /// cache buffer addresses on the remote core
      Grappa::impl::ReusePool< RDMABuffer, CountingSemaphore, remote_buffer_pool_size > remote_buffers_;

      ///
      /// another cache line
      ///
      Grappa::impl::MessageBase * received_messages_;
      int64_t pad2[7];

      
      CoreData() 
        : messages_()
        , prefetch_queue_() 
        , representative_core_(0)
        , remote_buffers_()
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

      Core * source_core_for_locale_;
      Core * dest_core_for_locale_;
      Core * core_partner_locales_;
      int core_partner_locale_count_;

      void compute_route_map();
      void draw_routing_graph();


      bool flushing_;

      /// Received (filled) buffers go here.
      Grappa::impl::ReuseList< RDMABuffer > received_buffer_list_;

      /// Send (empty) buffers go here.
      Grappa::impl::ReuseList< RDMABuffer > free_buffer_list_;

      /// per-core storage
      CoreData * cores_;


      /// Active message to walk a buffer of received deserializers/functors and call them.
      static void deserialize_buffer_am( gasnet_token_t token, void * buf, size_t size );
      int deserialize_buffer_handle_;
      
      /// Active message to deserialize/call the first entry of a buffer of received deserializers/functors
      static void deserialize_first_am( gasnet_token_t token, void * buf, size_t size );
      int deserialize_first_handle_;
      
      /// Active message to enqueue a buffer to be received
      static void enqueue_buffer_am( gasnet_token_t token, void * buf, size_t size );
      int enqueue_buffer_handle_;
      
      /// Chase a list of messages and serialize them into a buffer.
      /// Modifies pointer to list to support size-limited-ish aggregation
      /// TODO: make this a hard limit?
      char * aggregate_to_buffer( char * buffer, Grappa::impl::MessageBase ** message_ptr, size_t max = -1, size_t * count = NULL );
      
      // Deserialize and call a buffer of messages
      static char * deaggregate_buffer( char * buffer, size_t size );

      /// Grab a list of messages to send
      inline Grappa::impl::MessageList grab_messages( Core c ) {
        Grappa::impl::MessageList * dest_ptr = &cores_[ c ].messages_;
        Grappa::impl::MessageList old_ml, new_ml;

        do {
          // read previous value
          old_ml = *dest_ptr;
          new_ml.raw_ = 0;
          // insert current message
        } while( !__sync_bool_compare_and_swap( &(dest_ptr->raw_), old_ml.raw_, new_ml.raw_ ) );

        return old_ml;
      }

      bool send_would_block( Core core );

      /// Sender size of RDMA transmission.
      void send_rdma( Core core, Grappa::impl::MessageList ml, size_t estimated_size );
      void send_rdma_old( Core core, Grappa::impl::MessageList ml, size_t estimated_size );
      void send_medium( Core core, Grappa::impl::MessageList ml );

      void send_with_buffers( Core core,
                              MessageBase ** messages_to_send_ptr,
                              RDMABuffer * local_buf,
                              RDMABuffer * remote_buf,
                              size_t * serialized_count_p,
                              size_t * serialize_calls_p,
                              size_t * serialize_null_returns_p );


      void deliver_locally( Core core,
                            MessageBase * messages_to_send );

      /// task that is run to allocate space to receive a message      
      static void deaggregation_task( GlobalAddress< FullEmpty < ReceiveBufferInfo > > callback_ptr );

      /// Accept an empty buffer sent by a remote host
      void cache_buffer_for_core( Core owner, RDMABuffer * b );

      /// flush one destination
      bool flush_one( Core c ) {
        if( cores_[c].messages_.raw_ != 0 ) {
          Grappa::impl::MessageList ml = grab_messages( c );
          size_t size = cores_[c].prefetch_queue_[ ( ml.count_ ) % prefetch_dist ].size_;
          global_rdma_aggregator.send_rdma( c, ml, size );
          return true;
        } else {
          return false;
        }
      }

      /// Task that is constantly waiting to do idle flushes. This
      /// ensures we always have some sending resource available.
      void idle_flusher();

      /// Task that is constantly waiting to receive and
      /// deaggregate. This ensures we always have receiving resource
      /// available.
      void receive_worker();

      /// Condition variable used to signal flushing task.
      Grappa::ConditionVariable flush_cv_;

      /// flush interlock for receives
      bool disable_flush_;
      const size_t max_size_;
      bool interesting_;
      
    public:

      RDMAAggregator()
        : mycore_( -1 )
        , mynode_( -1 )
        , cores_per_node_( -1 )
        , total_cores_( -1 )
        , source_core_for_locale_( NULL )
        , dest_core_for_locale_( NULL )
        , core_partner_locales_( NULL )
        , core_partner_locale_count_( 0 )
        , flushing_( false )
        , received_buffer_list_()
        , free_buffer_list_()
        , cores_(NULL)
        , deserialize_buffer_handle_( -1 )
        , deserialize_first_handle_( -1 )
        , enqueue_buffer_handle_( -1 )
        , flush_cv_()
        , disable_flush_(false)
        , max_size_( 1 << 16 )
        , interesting_( false )
      {
        CHECK_LE( FLAGS_target_size, max_size_ );
      }

      /// initialize and register with communicator
      void init();
      void activate();
      void finish();

      void poll( ) {
        if( cores_[ Grappa::mycore() ].messages_.raw_ != 0 ) {
          Grappa::impl::MessageList ml = grab_messages( Grappa::mycore() );
          deliver_locally( Grappa::mycore(), get_pointer( &ml ) );
        }
      }

      /// Enqueue message to be sent
      inline void enqueue( Grappa::impl::MessageBase * m ) {
        app_messages_enqueue++;
        DVLOG(5) << __func__ << ": Enqueued message " << m << ": " << m->typestr();

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
          if( spawn_send && !disable_flush_ ) {
            swap_ml.raw_ = 0; // leave the list empty
          } else {
            // stitch in this message
            swap_ml = new_ml;
          }

          // now try to insert current message (and count attempt)
          app_messages_enqueue_cas++;
        } while( !__sync_bool_compare_and_swap( &(dest_ptr->raw_), old_ml.raw_, swap_ml.raw_ ) );

        /// is it time to flush?
        if( disable_flush_ || !spawn_send ) { // no
          // now fill in prefetch pointer and size (and make sure size doesn't overflow)
          dest->prefetch_queue_[ count % prefetch_dist ].size_ = size < max_size_ ? size : max_size_-1;
          set_pointer( &(dest->prefetch_queue_[ count % prefetch_dist ]), m );
        } else {            // yes
          rdma_capacity_flushes++;
          if( disable_flush_ && send_would_block( core ) ) {
            Grappa::privateTask( [core, new_ml, size] {
              global_rdma_aggregator.send_rdma( core, new_ml, size );
              });
          } else {
            global_rdma_aggregator.send_rdma( core, new_ml, size );
          }
        }
      }


      /// send a message that will be run in active message context. This requires very limited messages.
      void send_immediate( Grappa::impl::MessageBase * m ) {
        app_messages_immediate++;

        // create temporary buffer with one extra byte for terminator
        const size_t size = m->serialized_size() + 1;
        char buf[ size ] __attribute__ ((aligned (16)));

        // serialize to buffer
        Grappa::impl::MessageBase * tmp = m;
        while( tmp != nullptr ) {
          DVLOG(5) << __func__ << ": Serializing message from " << tmp;
          char * end = aggregate_to_buffer( &buf[0], &tmp, size );
          DVLOG(5) << __func__ << ": After serializing, pointer was " << tmp;
          DCHECK_EQ( end - buf, size ) << __func__ << ": Whoops! Aggregated message was too long to send as immediate";
          
          DVLOG(5) << __func__ << ": Sending " << end - buf
                   << " bytes of aggregated messages to " << m->destination_;

          // send
          global_communicator.send(  m->destination_, deserialize_first_handle_, buf, size );
        }
      }

      /// Flush one destination.
      void flush( Core c ) {
        rdma_requested_flushes++;
        flush_one( c );
      }

      /// Initiate an idle flush.
      void idle_flush() {
        poll();
        //Grappa::signal( &flush_cv_ );
      }

    };

    /// @}
  }
}

#endif
