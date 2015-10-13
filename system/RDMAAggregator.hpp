////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////


#ifndef __RDMAAGGREGATOR_HPP__
#define __RDMAAGGREGATOR_HPP__

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <boost/dynamic_bitset.hpp>

#include "Communicator.hpp"
#include "Worker.hpp"
#include "tasks/TaskingScheduler.hpp"
#include "Tasking.hpp"
#include "Addressing.hpp"
//#include "PerformanceTools.hpp"

#include "LocaleSharedMemory.hpp"

#include "MessageBase.hpp"
#include "RDMABuffer.hpp"

#include "ConditionVariableLocal.hpp"
#include "CountingSemaphoreLocal.hpp"
#include "FullEmptyLocal.hpp"

#include "ReusePool.hpp"
#include "ReuseList.hpp"

#include "Metrics.hpp"

#include "NTMessage.hpp"
#include "NTBuffer.hpp"

// #include <boost/interprocess/containers/vector.hpp>

DECLARE_int64( target_size );
DECLARE_int64( aggregator_target_size );
DECLARE_int64( aggregator_autoflush_ticks );
DECLARE_bool( enable_aggregation );

/// stats for application messages
GRAPPA_DECLARE_METRIC( SimpleMetric<int64_t>, app_messages_enqueue );
GRAPPA_DECLARE_METRIC( SimpleMetric<int64_t>, app_messages_enqueue_cas );
GRAPPA_DECLARE_METRIC( SimpleMetric<int64_t>, app_messages_immediate );

GRAPPA_DECLARE_METRIC( SummarizingMetric<int64_t>, app_nt_message_bytes );

/// stats for RDMA Aggregator events
GRAPPA_DECLARE_METRIC( SimpleMetric<int64_t>, rdma_capacity_flushes );
GRAPPA_DECLARE_METRIC( SimpleMetric<int64_t>, rdma_requested_flushes );

GRAPPA_DECLARE_METRIC( SimpleMetric<int64_t>, rdma_poll );
GRAPPA_DECLARE_METRIC( SimpleMetric<int64_t>, rdma_poll_send );
GRAPPA_DECLARE_METRIC( SimpleMetric<int64_t>, rdma_poll_receive );
GRAPPA_DECLARE_METRIC( SimpleMetric<int64_t>, rdma_poll_send_success );
GRAPPA_DECLARE_METRIC( SimpleMetric<int64_t>, rdma_poll_receive_success );
GRAPPA_DECLARE_METRIC( SimpleMetric<int64_t>, rdma_poll_yields );

GRAPPA_DECLARE_METRIC( SummarizingMetric<double>, rdma_local_delivery_time );


namespace Grappa {
  
  extern double tick_rate;

  typedef Core Core;

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

    static const int prefetch_dist = 6;
    static const int prefetch_type = 3; // 0 (non-temporal) or 3 (L1) are probably the best choice
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
      

      ///
      /// all this should be on one cache line
      ///

      /// head of message list for this core
      Grappa::impl::MessageList messages_;
      /// prefetch queue for this core
      Grappa::impl::PrefetchEntry prefetch_queue_[ prefetch_dist ];

      // lowest-numbered core that is in this core's locale
      Core representative_core_;
      Locale mylocale_;
      int32_t pad32;

      // sending thread blocks here until it has work to do
      ConditionVariable send_cv_;

      // remember when we were last sent
      Grappa::Timestamp last_sent_;

      ///
      /// another cache line
      ///

      /// holds buffer addresses on the remote core
      Grappa::impl::ReusePool< RDMABuffer, CountingSemaphore, remote_buffer_pool_size > remote_buffers_;

      ///
      /// another cache line
      ///

      /// racy updates, used only in source core for locale
      size_t locale_byte_count_;
      Grappa::Timestamp earliest_message_for_locale_;

      int64_t pad2[7];

      
      CoreData() 
        : messages_()
        , prefetch_queue_() 
        , representative_core_(0)
        , mylocale_(0)
        , send_cv_()
        , last_sent_(0)
        , remote_buffers_()
        , locale_byte_count_(0)
        , earliest_message_for_locale_(0)
      { }
    } __attribute__ ((aligned(64)));


    /// New aggregator design
    class RDMAAggregator {
      //private:
    public:
      
      Core mycore_;
      Core mynode_;
      Core cores_per_node_;
      Core total_cores_;

      Core * source_core_for_locale_;
      Core * dest_core_for_locale_;
      Core * core_partner_locales_;
      int core_partner_locale_count_;

      NTBuffer * ntbuffers_;
      boost::dynamic_bitset<> nt_mru_;

      void compute_route_map();
      void draw_routing_graph();
      void fill_free_pool( size_t num_buffers );


      bool flushing_;

      /// Received (filled) buffers go here.
      Grappa::impl::ReuseList< RDMABuffer > received_buffer_list_;

      /// Send (empty) buffers go here.
      Grappa::impl::ReuseList< RDMABuffer > free_buffer_list_;

      /// per-core storage
      CoreData * cores_;


      /// Active message to deserialize/call all the entries in a buffer of received deserializers/functors
      static void deserialize_buffer_am( void * buf, int size, CommunicatorContext * c );

      /// Active message to deserialize/call the first entry of a buffer of received deserializers/functors
      static void deserialize_first_am( void * buf, int size, CommunicatorContext * c );
      
      /// Active message to enqueue a buffer to be received
      static void enqueue_buffer_am( void * buf, int size, CommunicatorContext * c );

      /// buffers for message transmission
      RDMABuffer * rdma_buffers_;

      inline CoreData * coreData( Core c ) const {
        return &cores_[ Grappa::locale_mycore() * Grappa::cores() + c ]; 
      }

      inline CoreData * coreData( Core destination, Core locale_core ) const { 
        return &cores_[ locale_core * Grappa::cores() + destination ]; 
      }

      inline CoreData * localeCoreData( Core c ) const { 
        return &cores_[ Grappa::locale_cores() * Grappa::cores() + c ]; 
      }



      void send_nt_buffer( Core dest, NTBuffer * buf );
      void receive_nt_buffer( RDMABuffer * buf );






      /// Chase a list of messages and serialize them into a buffer.
      /// Modifies pointer to list to support size-limited-ish aggregation
      /// TODO: make this a hard limit?
      char * aggregate_to_buffer( char * buffer, Grappa::impl::MessageBase ** message_ptr, size_t max = -1, uint64_t * count = NULL );
      
      // Deserialize and call a buffer of messages
      static char * deaggregate_buffer( char * buffer, size_t size );

      /// Grab a list of messages to send
      inline Grappa::impl::MessageList grab_messages( Core c, Core sender ) {
        Grappa::impl::MessageList * dest_ptr = &(coreData( c, sender )->messages_);
        Grappa::impl::MessageList old_ml, new_ml;

        do {
          // read previous value
          old_ml = *dest_ptr;
          new_ml.raw_ = 0;
          // insert current message
        } while( !__sync_bool_compare_and_swap( &(dest_ptr->raw_), old_ml.raw_, new_ml.raw_ ) );

        // reset size (racily)
        coreData( c, sender )->prefetch_queue_[ 0 ].raw_ = 0;

        return old_ml;
      }

      inline Grappa::impl::MessageList grab_locale_messages( Core c ) {
        Grappa::impl::MessageList * dest_ptr = &(localeCoreData( c )->messages_);
        Grappa::impl::MessageList old_ml, new_ml;

        do {
          // read previous value
          old_ml = *dest_ptr;
          new_ml.raw_ = 0;
          // insert current message
        } while( !__sync_bool_compare_and_swap( &(dest_ptr->raw_), old_ml.raw_, new_ml.raw_ ) );

        // reset size (racily)
        localeCoreData(c)->prefetch_queue_[ 0 ].raw_ = 0;

        return old_ml;
      }

      inline Grappa::impl::MessageList grab_messages( CoreData * cd ) {
        Grappa::impl::MessageList * dest_ptr = &(cd->messages_);
        Grappa::impl::MessageList old_ml, new_ml;

        do {
          // read previous value
          old_ml = *dest_ptr;
          new_ml.raw_ = 0;
          // insert current message
        } while( !__sync_bool_compare_and_swap( &(dest_ptr->raw_), old_ml.raw_, new_ml.raw_ ) );
        
        // reset size (racily)
        cd->prefetch_queue_[ 0 ].raw_ = 0;

        return old_ml;
      }


      bool send_would_block( Core core );

      /// Sender size of RDMA transmission.
      void send_rdma( Core core, Grappa::impl::MessageList ml, size_t estimated_size );
      void send_rdma_old( Core core, Grappa::impl::MessageList ml, size_t estimated_size );
      void send_medium( Core core, Grappa::impl::MessageList ml );

      bool maybe_has_messages( Core c, Core locale_source );
      void issue_initial_prefetches( Core core, Core locale_source );
      void issue_initial_prefetches( CoreData * cd );
      void send_locale_medium( Locale locale );
      void send_locale( Locale locale );

      void send_with_buffers( Core core,
                              MessageBase ** messages_to_send_ptr,
                              RDMABuffer * local_buf,
                              RDMABuffer * remote_buf,
                              size_t * serialized_count_p,
                              size_t * serialize_calls_p,
                              size_t * serialize_null_returns_p );


      size_t deliver_locally( Core core,
                              Grappa::impl::MessageList ml,
                              CoreData * coreData );




      /// task that is run to allocate space to receive a message      
      static void deaggregation_task( GlobalAddress< FullEmpty < ReceiveBufferInfo > > callback_ptr );

      /// Accept an empty buffer sent by a remote host
      void cache_buffer_for_core( Core owner, RDMABuffer * b );


      /// do we have any work to do for a locale?
      bool check_for_work_on( Locale locale, size_t size) {
        // Core c = source_core_for_locale_[locale];
        // CHECK_NE( Grappa::mycore(), c );
        // size_t byte_count = cores_[c].locale_byte_count_;
        // return ( byte_count > size );
        Core c = locale * Grappa::locale_cores();

        // 
        CHECK_NE( Grappa::mycore(), c );

        // have we timed out?
        Grappa::Timestamp current_ts = Grappa::timestamp();
        if( current_ts - localeCoreData(c)->last_sent_ > FLAGS_aggregator_autoflush_ticks ) {
          return true;
        }

        // // have we reached a size limit?
        // size_t byte_count = localeCoreData(c)->locale_byte_count_;
        // return ( byte_count > size );
        return false;
      }

      bool check_for_any_work_on( Locale locale ) {
        Core start = locale * Grappa::locale_cores();
        Core max = start + Grappa::locale_cores();
        // for all the cores for this locale,
        for( Core locale_core = 0; locale_core < Grappa::locale_cores(); ++locale_core ) {
          // check each of my cores' lists
          for( Core c = start; c < max; ++c ) {
            if( coreData(c, locale_core)->messages_.raw_ != 0 ) {
              return true;
            }
          }
        }
        return false;
      }



      /// Task that is constantly waiting to do idle flushes. This
      /// ensures we always have some sending resource available.
      void idle_flusher();

      /// Task that is constantly waiting to receive and
      /// deaggregate. This ensures we always have receiving resource
      /// available.
      void send_worker( Locale locale );
      void receive_worker();
      void receive_buffer( RDMABuffer * b );

      /// Condition variable used to signal flushing task.
      Grappa::ConditionVariable flush_cv_;

      /// flush interlock for receives
      bool disable_flush_;
      const size_t max_size_;
      bool interesting_;
      
      uint64_t * enqueue_counts_;
      uint64_t * aggregate_counts_;
      uint64_t * deaggregate_counts_;

      int64_t active_receive_workers_;
      int64_t active_send_workers_;

    public:
      bool disable_everything_;

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
        , ntbuffers_( nullptr )
        , flushing_( false )
        , received_buffer_list_()
        , free_buffer_list_()
        , cores_(NULL)
        , rdma_buffers_( NULL )
        , flush_cv_()
        , disable_flush_(false)
        , max_size_( (1 << 16) )
        , interesting_( false )
        , enqueue_counts_( NULL )
        , aggregate_counts_( NULL )
        , deaggregate_counts_( NULL )
        , active_receive_workers_(0)
        , active_send_workers_(0)
        , disable_everything_( false )
      {
        CHECK_LE( FLAGS_target_size, max_size_ );
      }
      
      /// Adjust parameters to attempt to make this component use a given amount of memory.
      ///
      /// @param target  memory footprint (in bytes) that this should try to fit in
      /// @return actual size estimate (may be larger than target)
      size_t adjust_footprint(size_t target);
      
      /// Estimate amount of memory the communicator will use when 'activate()' is called
      size_t estimate_footprint() const;
      
      /// initialize and register with communicator
      void init();
      void activate();
      void finish();

      void dump_counts();

      bool receive_poll() {
        rdma_poll_receive++;
        //DVLOG(4) << "RDMA receive poll";
        bool useful = false;
        Core c = Grappa::mycore();
        // see if we have anything to receive

        // try global queue
        if( localeCoreData(c)->messages_.raw_ != 0 ) {
          useful = true;
            
          DVLOG(4) << "Polling found messages.raw " << (void*) localeCoreData(c)->messages_.raw_  
                   << " count " << localeCoreData(c)->messages_.count_ ;
            
          //Grappa::Timestamp start = Grappa::force_tick();
            
          Grappa::impl::MessageList ml = grab_locale_messages( c );
          size_t count = deliver_locally( c, ml, localeCoreData( c ) );

          //Grappa::Timestamp elapsed = Grappa::force_tick() - start;
          //rdma_local_delivery_time += (double) elapsed / tick_rate;
        }

        for( Core locale_source = 0; locale_source < Grappa::locale_cores(); ++locale_source ) {
          if( coreData(c, locale_source)->messages_.raw_ != 0 ) {
            useful = true;
            
            DVLOG(4) << "Polling found messages.raw " << (void*) coreData(c,locale_source)->messages_.raw_  
                     << " count " << coreData(c,locale_source)->messages_.count_ ;
            
            //Grappa::Timestamp start = Grappa::force_tick();
            
            Grappa::impl::MessageList ml = grab_messages( c, locale_source );
            size_t count = deliver_locally( c, ml, coreData( c, locale_source ) );

            //Grappa::Timestamp elapsed = Grappa::force_tick() - start;
            //rdma_local_delivery_time += (double) elapsed / tick_rate;
          }
        }
        if( useful ) rdma_poll_receive_success++;
        return useful;
      }

      bool send_poll() {
        rdma_poll_send++;
        bool useful = false;
        if( !disable_everything_ ) {
          // see if we have anything to send
          for( int i = 0; i < core_partner_locale_count_; ++i ) {
            Locale locale = core_partner_locales_[i];
            Core core = locale * Grappa::locale_cores();
            if( check_for_work_on( locale, FLAGS_target_size ) ) {
              useful = true;
              Grappa::signal( &(localeCoreData(core)->send_cv_)  );
              //send_locale_medium( locale );
            }
          }
        }
        if( useful ) rdma_poll_send_success++;
        return useful;
      }

      bool poll( ) {
        rdma_poll++;

        //DVLOG(4) << "RDMA poll";

        bool receive_success = receive_poll();
        bool send_success = send_poll();
        return receive_success || send_success;
      }

      /// Enqueue message to be sent
      inline void enqueue( Grappa::impl::MessageBase * m, bool locale_enqueue ) {
        app_messages_enqueue++;

        enqueue_counts_[ m->destination_ ]++;

        // don't yield too often.
        static int yield_wait = 2;
        if( !global_scheduler.in_no_switch_region() && !disable_everything_ && yield_wait-- == 0 ) {
          bool should_yield = Grappa::impl::global_scheduler.thread_maybe_yield();
          if( should_yield ) {
            DVLOG(3) << "Should yield";
            yield_wait = 2;
            Grappa::impl::global_scheduler.thread_yield();
            rdma_poll_yields++;
          }
            //if( yielded ) rdma_poll_yields++;
        }

        // static int freq = 10;
        // if( freq-- == 0 ) {
        //   poll();
        //   freq = 10;
        // }

        DVLOG(5) << __func__ << "/" << Grappa::impl::global_scheduler.get_current_thread() 
                 << ": Enqueued message " << m 
                 << " with is_delivered_=" << m->is_delivered_ 
                 << ": " << m->typestr();

        // get destination pointer
        Core core = m->destination_;
        CoreData * dest = NULL;

        if( locale_enqueue ) {
          dest = localeCoreData(core);
        } else {
          dest = coreData(core);
        }

        //CoreData * sender = &cores_[ dest->representative_core_ ];
        CoreData * locale_core = localeCoreData( Grappa::locale_of( m->destination_ ) * Grappa::locale_cores() );

        // possibly short circuit out of here when aggregation is disabled
        if( !FLAGS_enable_aggregation &&
            !global_scheduler.in_no_switch_region() &&
            Grappa::locale_of( m->destination_ ) != Grappa::mylocale() &&
            global_communicator.send_context_available() ) {
          return send_immediate( m );
        }


        //Grappa::impl::MessageBase ** dest_ptr = &dest->messages_;
        Grappa::impl::MessageList * dest_ptr = &(dest->messages_);
        Grappa::impl::MessageList old_ml, new_ml, swap_ml;

        // new values computed from previous totals
        int count = 0;
        size_t size =  m->serialized_size();
        swap_ml.raw_ = 0;

        bool spawn_send = false;

        // prepare to stitch in message
        set_pointer( &new_ml, m );

        int cas_count = 0;
        do {
          // read previous value
          old_ml = *dest_ptr;

          // add previous count/estimated size
          count = 1 + old_ml.count_;
          new_ml.count_ = count; 

          // append previous list to current message
          m->next_ = get_pointer( &old_ml );
          // set prefetch to the oldest pointer we remember
          // index prefetch queue by count since we haven't overwritten it in yet.
          m->prefetch_ = get_pointer( &(dest->prefetch_queue_[ count % prefetch_dist ]) );
          //m->prefetch_ = NULL;

          size += dest->prefetch_queue_[ count % prefetch_dist ].size_;

          // now try to insert current message (and count attempt)
          cas_count++;
        } while( !__sync_bool_compare_and_swap( &(dest_ptr->raw_), old_ml.raw_, new_ml.raw_ ) );
        // } while( false );
        // *dest_ptr = new_ml;
        
        app_messages_enqueue_cas += cas_count;

        // // warning: racy
        // locale_core->locale_byte_count_ += size;
          
        dest->prefetch_queue_[ count % prefetch_dist ].size_ = size < max_size_ ? size : max_size_-1;
        set_pointer( &(dest->prefetch_queue_[ count % prefetch_dist ]), m );

//         // possibly flush if we've passed target size
//         if( FLAGS_target_size > 0 &&
//             size > FLAGS_target_size &&
//             !locale_enqueue &&
//             !global_scheduler.in_no_switch_region() &&
//             Grappa::locale_of( m->destination_ ) != Grappa::mylocale() &&
//             global_communicator.send_context_available() ) {
//           dest = coreData(core);
//           auto ml = grab_messages( dest );
//           auto mp = get_pointer( &ml );
//           rdma_capacity_flushes++;
//           if( mp ) {
//             return send_list_immediate( mp );
//           }
//         }
      }


      /// send a message that will be run in active message context. This requires very limited messages.
      void send_list_immediate( Grappa::impl::MessageBase * m ) {
        app_messages_immediate++;

        const size_t size = m->serialized_size();

        CommunicatorContext * c = global_communicator.try_get_send_context();
        if( !c ) {
          global_communicator.garbage_collect();
          c = global_communicator.try_get_send_context();
        }
        while( !c ) {
          if( global_scheduler.in_no_switch_region() ) {
            global_communicator.poll();
          } else {
            Grappa::yield();
          }
          global_communicator.garbage_collect();
          c = global_communicator.try_get_send_context();
        }

        Core dest = m->destination_;
        char * buf = (char*) c->buf;
        DCHECK_LE( size, c->size );

        // write deserializer pointer
        typedef decltype(&deserialize_first_am) Deserializer;
        Deserializer * dbuf = (Deserializer*) buf;
        *dbuf = &deserialize_buffer_am;
        buf = (char*) (dbuf + 1);

        size_t remaining = c->size - (buf - ((char*)c->buf));

        // serialize to buffer
        Grappa::impl::MessageBase * tmp = m;
        while( tmp != nullptr ) {
          DVLOG(5) << __func__ << ": Serializing message from " << tmp;
          char * end = aggregate_to_buffer( buf, &tmp, remaining );
          DVLOG(5) << __func__ << ": After serializing, pointer was " << tmp;
          DCHECK_EQ( end - buf, size ) << __func__ << ": Whoops! Aggregated message was too long to send as immediate";
          
          DVLOG(5) << __func__ << ": Sending " << end - buf
                   << " bytes of aggregated messages to " << dest;

          c->callback = NULL;
          global_communicator.post_send( c, dest, end-((char*)c->buf) );
        }
      }

      /// send a message that will be run in active message context. This requires very limited messages.
      void send_immediate( Grappa::impl::MessageBase * m ) {
        app_messages_immediate++;

        const size_t size = m->serialized_size();

        global_communicator.garbage_collect();
        CommunicatorContext * c = global_communicator.try_get_send_context();
        if( !c ) {
          global_communicator.garbage_collect();
          c = global_communicator.try_get_send_context();
        }
        while( !c ) {
          if( global_scheduler.in_no_switch_region() ) {
            global_communicator.poll();
          } else {
            Grappa::yield();
          }
          global_communicator.garbage_collect();
          c = global_communicator.try_get_send_context();
        }

        Core dest = m->destination_;
        char * buf = (char*) c->buf;
        DCHECK_LE( size, c->size );

        // write deserializer pointer
        typedef decltype(&deserialize_first_am) Deserializer;
        Deserializer * dbuf = (Deserializer*) buf;
        *dbuf = &deserialize_first_am;
        buf = (char*) (dbuf + 1);

        size_t remaining = c->size - (buf - ((char*)c->buf));

        // serialize to buffer
        Grappa::impl::MessageBase * tmp = m;
        while( tmp != nullptr ) {
          DVLOG(5) << __func__ << ": Serializing message from " << tmp;
          char * end = aggregate_to_buffer( buf, &tmp, remaining );
          DVLOG(5) << __func__ << ": After serializing, pointer was " << tmp;
          DCHECK_EQ( end - buf, size ) << __func__ << ": Whoops! Aggregated message was too long to send as immediate";
          
          DVLOG(5) << __func__ << ": Sending " << end - buf
                   << " bytes of aggregated messages to " << dest;

          c->callback = NULL;
          global_communicator.post_send( c, dest, end-((char*)c->buf) );
        }
      }

      /// Flush one destination.
      void flush( Core c ) {
        rdma_requested_flushes++;
        Locale locale = Grappa::locale_of(c);
        if( source_core_for_locale_[ locale ] == Grappa::mycore() ) {
          Grappa::signal( &(localeCoreData( locale * Grappa::locale_cores() )->send_cv_) );
        } else {
          // not on our core, so we can't signal it. instead, cause
          // polling thread to detect timeout.
          localeCoreData( locale * Grappa::locale_cores() )->last_sent_ = 0;
        }
      }

      /// Initiate an idle flush.
      void idle_flush() {
        Grappa::signal( &flush_cv_ );
      }

      template< typename T >
      inline void send_nt_message( Core dest, T t ) {
        NTMessage<T> m( dest, t );
        DVLOG(3) << "Sending " << sizeof(m) << " bytes to " << dest;
        app_nt_message_bytes += sizeof(m);
        int size = nt_enqueue( ntbuffers_ + dest, &m, sizeof(m) );

        // update mru
        nt_mru_.set(dest);

        // send if we've reached capacity
        if( size >= (FLAGS_aggregator_target_size + 4 * sizeof(uint64_t)) ) { // TODO: magic number
          send_nt_buffer( dest, ntbuffers_ + dest );
        }
      }

      inline void flush_nt( Core dest ) {
        nt_flush( ntbuffers_ + dest );
        send_nt_buffer( dest, ntbuffers_ + dest );
      }
    };

    /// @}
  }
}

#endif
