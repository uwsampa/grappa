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


#include <boost/test/unit_test.hpp>
#include <cstdlib>

#include "Grappa.hpp"
#include "RDMAAggregator.hpp"
#include "Message.hpp"
#include "CompletionEvent.hpp"
#include "Metrics.hpp"
#include "ParallelLoop.hpp"
#include "ReuseMessage.hpp"
#include "ReuseMessageList.hpp"
#include "RDMABuffer.hpp"

#include <algorithm>

DEFINE_int64( iterations_per_core, 1 << 24, "Number of messages sent per core" );
DEFINE_int64( outstanding, 1 << 13, "Number of messages enqueued and unsent during aggregation test" );
DEFINE_bool( disable_sending, false, "Disable sending during aggregation test" );
DEFINE_bool( disable_switching, false, "Disable context switching during local delivery test" );

DEFINE_bool( send_to_self, false, "Send only to self during local delivery test" );

DEFINE_bool( aggregate_source_multiple, false, "Aggregate from multiple source cores during aggregation test" );
DEFINE_bool( aggregate_dest_multiple, false, "Aggregate to multiple destination cores during aggregation test" );

DEFINE_int64( sender_override, 0, "Override core_partner_locale_count_-based decision about number of senders in remote distribution test; if set, use this many" );

DEFINE_string( mode, "serialization", "Which test to run: local, serialization, aggregation, distribution");

DEFINE_int64( seed, -1, "RNG seed for serialization test" );
DEFINE_bool( permute, true, "Permute messages in serialization test" );
DEFINE_int64( prefetch_distance, 4, "Prefetch distance for serialization test" );
DEFINE_bool( prefetch_enable, false, "Prefetch for serialization test" );

DECLARE_int64( rdma_buffers_per_core );

DECLARE_int64( loop_threshold );

BOOST_AUTO_TEST_SUITE( RDMAAggregator_tests );


int count = 0;
int count2 = 0;

Grappa::CompletionEvent local_ce;
size_t local_count = 0;

GRAPPA_DEFINE_METRIC( SimpleMetric<double>, enqueued_messages_rate_per_core, 0.0 );

GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, local_messages_per_locale, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, local_messages_time, 0.0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, local_messages_rate_per_locale, 0.0 );

GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, remote_distributed_messages_per_locale, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, remote_distributed_buffers_per_locale, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, remote_distributed_messages_time, 0.0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, remote_distributed_messages_rate_per_locale, 0.0 );

GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, serialized_messages_per_locale, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, serialized_messages_time, 0.0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, serialized_messages_rate_per_locale, 0.0 );

GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, deserialized_messages_per_locale, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, deserialized_messages_time, 0.0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, deserialized_messages_rate_per_locale, 0.0 );

GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, aggregated_messages_per_locale, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, aggregated_messages_time, 0.0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, aggregated_messages_rate_per_locale, 0.0 );



BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{
    LOG(INFO) << "Useless: " << FLAGS_flatten_completions << ", " << FLAGS_loop_threshold;

    if(false) {
    LOG(INFO) << "Test 1";
    if (true) {
      auto m = Grappa::message( 0, []{ LOG(INFO) << "Test message 1a: count = " << count++; } );
      auto n = Grappa::message( 0, []{ LOG(INFO) << "Test message 2a: count = " << count++; } );
      auto o = Grappa::message( 1, []{ LOG(INFO) << "Test message 3a: count = " << count++; } );
      auto p = Grappa::message( 1, []{ LOG(INFO) << "Test message 4a: count = " << count++; } );

      LOG(INFO) << "Sending m";
      m.send_immediate();
      LOG(INFO) << "Sending n";
      n.send_immediate();
      o.send_immediate();
      p.send_immediate();
    }

    LOG(INFO) << "Test 2";
    if (true) {
      auto o = Grappa::message( 1, []{ LOG(INFO) << "Test message 1b: count = " << count++; } );
      auto p = Grappa::message( 1, []{ LOG(INFO) << "Test message 2b: count = " << count++; } );
      count += 2;
  
      o.enqueue();
      p.enqueue();

      LOG(INFO) << "Flushing 1";
      Grappa::impl::global_rdma_aggregator.flush( 1 );
    }

    LOG(INFO) << "Test 3";
    if (true) {
      auto m = Grappa::send_message( 0, []{ LOG(INFO) << "Test message 1c: count = " << count++; } );
      auto n = Grappa::send_message( 0, []{ LOG(INFO) << "Test message 2c: count = " << count++; } );
      auto o = Grappa::send_message( 1, []{ LOG(INFO) << "Test message 3c: count = " << count++; } );
      auto p = Grappa::send_message( 1, []{ LOG(INFO) << "Test message 4c: count = " << count++; } );

      LOG(INFO) << "Flushing 0";
      Grappa::impl::global_rdma_aggregator.flush( 0 );

      LOG(INFO) << "Flushing 1";
      Grappa::impl::global_rdma_aggregator.flush( 1 );
    }


    LOG(INFO) << "Test 4";
    if (true) {
      const int iters = 4000;
      const int msgs = 3000;
      const int payload_size = 8;

      Grappa::ConditionVariable cv;
      char payload[ 1 << 16 ];

      struct F {
        Grappa::ConditionVariable * cvp;
        size_t total;
        //       void operator()() {
        // Grappa::ConditionVariable * cvpp = cvp;
        //        ++count2;
        //        if( count2 == total ) {
        //          auto reply = Grappa::message( 0, [cvpp] {
        //              Grappa::signal( cvpp );
        //            });
        //          reply.send_immediate();
        //        }
        //       }
        void operator()( void * payload, size_t size ) {
          Grappa::ConditionVariable * cvpp = cvp;
          ++count2;
          if( count2 == total ) {
            auto reply = Grappa::message( 0, [cvpp] {
                Grappa::signal( cvpp );
              });
            reply.send_immediate();
          }
        }
      };

      Grappa::PayloadMessage< F > m[ msgs ];
      
      double start = Grappa::walltime();
    
      for( int j = 0; j < iters; ++j ) {
        //LOG(INFO) << "Iter " << j;
        for( int i = 0; i < msgs; ++i ) {
          m[i].reset();
          m[i].set_payload( &payload[0], payload_size );
          m[i]->total = iters * msgs;
          m[i]->cvp = &cv;
          m[i].enqueue( 1 );
        }
      
        Grappa::impl::global_rdma_aggregator.flush( 1 );
      }
    
      //      Grappa::impl::global_rdma_aggregator.flush( 1 );
      //Grappa::wait( &cv );
    
      double time = Grappa::walltime() - start;
      size_t size = m[0].size();
      LOG(INFO) << time << " seconds, message size " << size << "; Throughput: " 
                << (double) iters * msgs / time << " iters/s, "
                << (double) size * iters * msgs / time << " bytes/s, "
                << (double) payload_size * iters * msgs / time << " payload bytes/s, ";
    }

    }
    // make earlier tests shut up
    Grappa::on_all_cores( [] { count = 6; } );


  
  

  
    // test local message delivery
    if( FLAGS_mode.compare("local") == 0 ) {
      LOG(INFO) << "Testing local message delivery";
      // const int64_t expected_messages_per_core = Grappa::locale_cores() * FLAGS_iterations_per_core;
      // const int64_t expected_messages_per_locale = expected_messages_per_core * Grappa::locale_cores();
    
      // const int64_t sent_messages_per_core = FLAGS_iterations_per_core / Grappa::locale_cores();
      // const int64_t expected_messages_per_core = sent_messages_per_core * Grappa::locale_cores();
      // // const int64_t sent_messages_per_core = FLAGS_iterations_per_core;
      // // const int64_t expected_messages_per_core = sent_messages_per_core;
      // const int64_t expected_messages_per_locale = expected_messages_per_core * Grappa::locale_cores();

      const int64_t sent_messages_per_core = FLAGS_iterations_per_core / Grappa::locale_cores() / Grappa::locale_cores();
      const int64_t expected_messages_per_core = sent_messages_per_core * Grappa::locale_cores();
      const int64_t expected_messages_per_locale = expected_messages_per_core * Grappa::locale_cores();

      struct LocalDelivery {
        Core source_core;
        Core dest_core;
        void operator()() { 
          local_count++; 
          local_ce.complete(); 
        }
      };
    
      double start = Grappa::walltime();

      Grappa::Metrics::start_tracing();
      
      Grappa::on_all_cores( [expected_messages_per_core, sent_messages_per_core] {
          
          LOG(INFO) << __PRETTY_FUNCTION__ << ": Starting.";
          google::FlushLogFiles(google::GLOG_INFO);

          // Grappa::ReuseMessage< LocalDelivery > m1;
          // m1->source_core = 1234;
          // m1->dest_core = Grappa::mycore();
          // local_ce.enroll( 1 );
          // m1.deliver_locally();
        
          LOG(INFO) << __PRETTY_FUNCTION__ << ": Constructing.";
          google::FlushLogFiles(google::GLOG_INFO);
          Grappa::ReuseMessageList< LocalDelivery > msgs( FLAGS_outstanding );
          LOG(INFO) << __PRETTY_FUNCTION__ << ": Activating.";
          google::FlushLogFiles(google::GLOG_INFO);
          msgs.activate(); // allocate messages

          { 
            local_ce.reset();
            local_count = 0;
            local_ce.enroll( expected_messages_per_core );
            LOG(INFO) << "Core " << Grappa::mycore() << " waiting for " << local_ce.get_count() << " updates.";

            Grappa::barrier();


            if( FLAGS_disable_sending ) {
              // keep aggregator from polling during sends or flushing for a bit.
              Grappa::impl::global_rdma_aggregator.disable_everything_ = true;
            }

            if( FLAGS_disable_switching ) {
              Grappa::impl::global_scheduler.set_no_switch_region( true );
              LOG(INFO) << "switching disabled";
            }

            LOG(INFO) << "sending messages";

            for( int i = 0; i < sent_messages_per_core; ++i ) {
              for( int locale_core = 0; locale_core < Grappa::locale_cores(); ++locale_core ) {
              // Core locale_core = Grappa::locale_cores() - Grappa::mycore() - 1;
                Core mycore = Grappa::mycore();
                Core dest_core = locale_core + Grappa::mylocale() * Grappa::locale_cores();
                if( FLAGS_send_to_self ) {
                  dest_core = mycore;
                }

                //Core dest_core = (Grappa::locale_cores() - locale_core - 1) + Grappa::mylocale() * Grappa::locale_cores();
                msgs.with_message( [mycore, dest_core] ( Grappa::ReuseMessage<LocalDelivery> * m ) {
                    (*m)->source_core = mycore;
                    (*m)->dest_core = dest_core;
                    CHECK_EQ( Grappa::locale_of( mycore ), Grappa::locale_of( dest_core ) );
                    m->enqueue( dest_core );
                  } );
                }
            }

            if( FLAGS_disable_switching ) {
              Grappa::impl::global_scheduler.set_no_switch_region( false );
              LOG(INFO) << "switching enabled";
            }

            LOG(INFO) << "delivering messages";

            for( int locale_core = 0; locale_core < Grappa::locale_cores(); ++locale_core ) {
              Grappa::impl::global_rdma_aggregator.flush( locale_core + Grappa::mylocale() * Grappa::locale_cores() );
            }

            Grappa::yield();

            if( FLAGS_disable_sending ) {
              // keep aggregator from polling during sends or flushing for a bit.
              Grappa::impl::global_rdma_aggregator.disable_everything_ = false;
            }

            local_ce.wait();

            Grappa::barrier();

            BOOST_CHECK_EQUAL( local_count, expected_messages_per_core );
            CHECK_EQ( local_count, expected_messages_per_core ) << "Local message delivery";
          }
        
          msgs.finish(); // clean up messages
        } );
      Grappa::Metrics::stop_tracing();

      double time = Grappa::walltime() - start;
      local_messages_per_locale = expected_messages_per_locale;
      local_messages_time = time;
      local_messages_rate_per_locale = expected_messages_per_locale / time;
    }









    // test serialization
    if( FLAGS_mode.compare("serialization") == 0 ) {
      LOG(INFO) << "Testing message serialization";

      unsigned int seed = FLAGS_seed == -1 ? time( NULL ) : FLAGS_seed;
      LOG(INFO) << "Seed is " << seed;
      srandom( seed );

      // no cross-core communication in this test
      const int64_t sent_messages_per_core = FLAGS_iterations_per_core; // / Grappa::locale_cores() / Grappa::locale_cores();
      const int64_t expected_messages_per_core = sent_messages_per_core; // * Grappa::locale_cores();
      const int64_t expected_messages_per_locale = expected_messages_per_core; // * Grappa::locale_cores();

      struct SerializedFunctor {
        int64_t one;
        int64_t three;
        void operator()() { 
          local_count++; 
        }
      };
    
      // message to increment count
      typedef Grappa::Message< SerializedFunctor > SerializedMessage;
    
      // number of messages
      const size_t num_messages = expected_messages_per_core;
    
      {
        void * ptr = NULL;
        posix_memalign( &ptr, 64, sizeof(SerializedMessage) * num_messages );
        CHECK_NOTNULL( ptr );
        SerializedMessage * msgs = new (ptr) SerializedMessage[ num_messages ];
        std::unique_ptr< int[] > msg_indexes( new int[ num_messages ] );
      
        // initialize
        local_ce.reset();
        for( size_t i = 0; i < num_messages; ++i ) {
          msg_indexes[i] = i;
          msgs[i].reset();
          msgs[i].source_ = Grappa::mycore();
          msgs[i].destination_ = Grappa::mycore();
        }
    
        // permute indices
        if( FLAGS_permute ) {
          for( size_t i = num_messages - 1; i > 0; --i ) {
            uint64_t x = random() % i;
            int temp = msg_indexes[i];
            msg_indexes[i] = msg_indexes[x];
            msg_indexes[x] = temp;
          }
        }

        // stitch together
        for( size_t i = 0; i < num_messages; ++i ) {
          CHECK_NE( i, msg_indexes[i] ) << "Can't point a message at itself";
          DVLOG(3) << "Message " << i << " links with " << msg_indexes[i];

          // set next pointer
          if( msg_indexes[i] > 0 ) {
            msgs[i].next_ = &msgs[ msg_indexes[i] ];
          }

          msgs[i].is_enqueued_ = true;
        }
      
        // stitch prefetch chain
        Grappa::impl::MessageBase * current = &msgs[0];
        Grappa::impl::MessageBase * prefetch = &msgs[0];
        for( size_t i = 0; i < num_messages; ++i ) {
          // set previous prefetch pointer
          if( FLAGS_prefetch_enable ) {
            if( i > FLAGS_prefetch_distance ) {
              prefetch->prefetch_ = current;
              prefetch = prefetch->next_;
            }
          }
          current = current->next_;
        }

        // serialize
        LOG(INFO) << "Now serializing.";
        Grappa::impl::MessageBase * first = &msgs[0];
        const size_t sizeof_messages = first->serialized_size() * num_messages;
      
        std::unique_ptr< char[] > buf( new char[ sizeof_messages ] );
      

        {
          Grappa::Metrics::start_tracing_here();
          double start = Grappa::walltime();

          {
            // serialize
            uint64_t count = 0;
            Grappa::impl::global_rdma_aggregator.aggregate_to_buffer( &buf[0], &first, sizeof_messages, &count);
          }
      
          double time = Grappa::walltime() - start;
          Grappa::Metrics::stop_tracing_here();
          serialized_messages_per_locale = expected_messages_per_locale;
          serialized_messages_time = time;
          serialized_messages_rate_per_locale = expected_messages_per_locale / time;
        }      

        {
          Grappa::Metrics::start_tracing_here();
          double start = Grappa::walltime();

          {
            // deserialize
            Grappa::impl::global_rdma_aggregator.deaggregate_buffer( &buf[0], sizeof_messages );
          }
      
          double time = Grappa::walltime() - start;
          Grappa::Metrics::stop_tracing_here();
          deserialized_messages_per_locale = expected_messages_per_locale;
          deserialized_messages_time = time;
          deserialized_messages_rate_per_locale = expected_messages_per_locale / time;
        }
    
      }

    }



    // test aggregation
    if( FLAGS_mode.compare("aggregation") == 0 ) {
      CHECK_EQ( Grappa::locales(), 2 ) << "Must have exactly two locales for this test";

      LOG(INFO) << "Testing aggregation and transmission";

      struct AggregatedMessage {
        Core source_core;
        Core dest_core;
        void operator()() { 
          local_count++; 
          local_ce.complete(); 
        }
      };


      double start = Grappa::walltime();

      // first test: single sending core, single receiving core. the sending core is responsible for aggregation.
      Grappa::on_all_cores( [] {

          const Locale source_locale = 0;
          const Locale dest_locale = 1;
          int64_t expected_messages_per_core = 0;

          if( dest_locale == Grappa::mylocale() ) { // locale 0 sends, locale 1 receives
            if( FLAGS_aggregate_dest_multiple ||                                       // either select all locale cores or
                Grappa::impl::global_rdma_aggregator.core_partner_locale_count_ > 0 ) { // select only the partner cores

              // if there's only one sender, this is the count
              expected_messages_per_core = FLAGS_iterations_per_core;
         
              // but if there are muliple, we need to increase it.
              if( FLAGS_aggregate_source_multiple && !FLAGS_aggregate_dest_multiple ) {
                expected_messages_per_core *= Grappa::locale_cores();
              }
            }
          }

          int64_t sent_messages_per_core = FLAGS_iterations_per_core;
          if( !FLAGS_aggregate_source_multiple && FLAGS_aggregate_dest_multiple ) {
            sent_messages_per_core *= Grappa::locale_cores();
          }

          local_ce.reset();
          local_count = 0;
          local_ce.enroll( expected_messages_per_core );

          Grappa::barrier();

          if( FLAGS_disable_sending ) {
            // keep aggregator from polling during sends or flushing for a bit.
            Grappa::impl::global_rdma_aggregator.disable_everything_ = true;
          }

          // now send messages
          // locale 0 sends, locale 1 receives
          if( source_locale == Grappa::mylocale() ) {

            if( FLAGS_aggregate_source_multiple ||                                     // either select all locale cores or
                Grappa::impl::global_rdma_aggregator.core_partner_locale_count_ > 0 ) { // select only the partner cores

              // prepare pool of messages
              size_t message_pool_size = FLAGS_outstanding;
              //CHECK_LT( FLAGS_iterations_per_core, message_pool_size ) << "don't try this yet";
              Grappa::ReuseMessageList< AggregatedMessage > msgs( message_pool_size );
              msgs.activate(); // allocate messages

              {
                const Core mycore = Grappa::mycore();
                Core dest_core = Grappa::impl::global_rdma_aggregator.dest_core_for_locale_[ dest_locale ];

                if( FLAGS_disable_sending ) {
                  // keep aggregator from polling during sends or flushing for a bit.
                  Grappa::impl::global_rdma_aggregator.disable_everything_ = true;

                  LOG(INFO) << "Sends disabled";
                }

                LOG(INFO) << "Now generating messages";              
                double send_start = Grappa::walltime();
              
                // send messages
                for( int i = 0; i < sent_messages_per_core; ++i ) {
                  if( FLAGS_aggregate_dest_multiple ) {
                    // rotate destination on remote node
                    dest_core = ( dest_locale * Grappa::locale_cores() + 
                                  (Grappa::locale_mycore() + i) % Grappa::locale_cores() );
                  }
                
                  //CHECK_EQ( Grappa::locale_of( dest_core ), dest_locale );
                  //CHECK_NE( Grappa::locale_of( dest_core ), Grappa::mylocale() );

                  msgs.with_message( [mycore, dest_core] ( Grappa::ReuseMessage<AggregatedMessage> * m ) {
                      (*m)->source_core = mycore;
                      (*m)->dest_core = dest_core;
                      m->enqueue( dest_core );
                    } );
                }
              

                double send_end = Grappa::walltime() - send_start;
                enqueued_messages_rate_per_core = sent_messages_per_core / send_end;

                if( FLAGS_disable_sending ) {
                  //sleep(10);
                }

                LOG(INFO) << "Now flushing";              
              
                // now send explicitly
                Grappa::impl::global_rdma_aggregator.flush( dest_core );
                Grappa::yield();

                if( FLAGS_disable_sending ) {
                  //sleep(30);
                
                  // reenable sending/flushing for this core
                  Grappa::impl::global_rdma_aggregator.disable_everything_ = false;
                }
              }

              msgs.finish();
            }
          }

          if( FLAGS_disable_sending ) {
              //sleep(60);
          }

          // wait for all completions.
          local_ce.wait();

          if( FLAGS_disable_sending ) {
            LOG(INFO) << "Now re-enabling sending for everybody";
            // reenable sending/flushing for everyone
            Grappa::impl::global_rdma_aggregator.disable_everything_ = false;
          }
        
          Grappa::barrier();

          BOOST_CHECK_EQUAL( local_count, expected_messages_per_core );
          CHECK_EQ( local_count, expected_messages_per_core ) << "Remote message distribution";

        });

      double time = Grappa::walltime() - start;
      aggregated_messages_per_locale = FLAGS_iterations_per_core;
      aggregated_messages_time = time;
      aggregated_messages_rate_per_locale = FLAGS_iterations_per_core / time;

    }



    // test message distribution
    if( FLAGS_mode.compare("distribution") == 0 ) {
      //CHECK_GE( Grappa::locales(), 2 ) << "Must have at least two locales for this test";
      CHECK_LE( Grappa::locales(), Grappa::locale_cores() ) 
        << "Haven't tested this test with locales>locale_cores. You'll need to adjust the expected message counts / send count";
    
      LOG(INFO) << "Testing remote message distribtion";
      //size_t num_sender_cores = std::min( Grappa::locale_cores(), static_cast<Locale>( Grappa::locales() - 1 ) );
      size_t num_sender_cores = FLAGS_sender_override > 0 ? 
        FLAGS_sender_override :
        std::min( Grappa::locale_cores(), static_cast<Locale>( Grappa::locales() - 1 ) );

      if( num_sender_cores == 0 ) {
        LOG(WARNING) << "Since there are no senders, I'm bumping num_sender_cores to 1. Use the flag to override.";
        num_sender_cores = 1;
      }

      const int64_t sent_messages_per_core = FLAGS_iterations_per_core / Grappa::locale_cores() / num_sender_cores;
      const int64_t expected_messages_per_core = sent_messages_per_core * num_sender_cores;
      const int64_t expected_messages_per_locale = expected_messages_per_core * Grappa::locale_cores();

      LOG(INFO) << num_sender_cores << " cores per locale sending "
                << sent_messages_per_core << " each, expecting "
                << expected_messages_per_core << " per core and " 
                << expected_messages_per_locale << " per locale.";

      double start = Grappa::walltime();
    
      Grappa::on_all_cores( [sent_messages_per_core, expected_messages_per_core, expected_messages_per_locale] {
			    
           bool i_am_sender = false;
  	if( FLAGS_sender_override > 0 ) {
  	  i_am_sender = Grappa::locale_mycore() < FLAGS_sender_override;
            Grappa::impl::global_rdma_aggregator.fill_free_pool( FLAGS_sender_override * FLAGS_rdma_buffers_per_core );
  	} else {
  	  i_am_sender = Grappa::impl::global_rdma_aggregator.core_partner_locale_count_ > 0;
  	}

          local_ce.reset();
          local_count = 0;
          local_ce.enroll( expected_messages_per_core );
          //LOG(INFO) << "Core " << Grappa::mycore() << " waiting for " << local_ce.get_count() << " updates.";

          const Core locale_core = Grappa::mylocale() * Grappa::locale_cores();
          const Core source_core = Grappa::mycore();

          Grappa::barrier();

          // if we are a communicating core
          if( i_am_sender ) {
            VLOG(5) << "Core " << Grappa::mycore() << " constructing buffers";

            // track how many messages we've sent to each core
            size_t num_sent[ Grappa::locale_cores() ];
            for( Core c = 0; c < Grappa::locale_cores(); ++c ) {
              num_sent[c] = 0;
            }
          
            // send as many messages to each core as expected
            bool done = false;
            while( !done ) {

              // get a buffer. we will fill this with an equal number of messages for each core
              VLOG(4) << "Grabbing a buffer";
              Grappa::impl::RDMABuffer * b = Grappa::impl::global_rdma_aggregator.free_buffer_list_.block_until_pop();
              CHECK_NOTNULL( b );

              // clear out buffer's count array
              for( Core c = 0; c < Grappa::locale_cores(); ++c ) {
                (b->get_counts())[c] = 0;
              }

              // set buffer source so we don't try to ack.
              b->set_source( Grappa::mycore() );

              // figure out how many bytes each core gets
              const size_t available = b->get_max_size();
              size_t available_each_core = available / Grappa::locale_cores();

              // start writing at start of buffer
              char * current = b->get_payload();
              char * previous = current;

              // write each core's contribution 
              for( Core c = 0; c < Grappa::locale_cores(); ++c ) {
                const Core dest_core = c + locale_core;

                // write messages until we run out of space
                size_t remaining_size = available_each_core;
                size_t num_serialized = 0;
                while( remaining_size > 0 && num_sent[ c ] < sent_messages_per_core ) {
                  // we will most likely break out of here before this test is false

                  // here's a message
                  VLOG(5) << "Sending a message to " << dest_core;
                  auto m = Grappa::message( dest_core, [] {
                      local_count++;
                      local_ce.complete();
                    });

                  // put the message in the buffer
                  size_t remaining_size = available_each_core - (current - previous);
                  char * end = m.serialize_to( current, remaining_size );

                  // did it fit?
                  if( current == end ) {
                    // nope, so break out of while loop and do next core.
                    break;
                  } else {
                    // yep, so update pointer and remaining size
                    current = end;
                    remaining_size -= end - current;
                    num_sent[ c ]++;
                  }
                }

                // mark number of bytes for this core in the buffer
                (b->get_counts())[c] = current - previous;
                previous = current;
              }

              // verify we haven't overrun the buffer
              CHECK_LE( current - b->get_payload(), available ) << "overran buffer!";
            
              // are we done?
              done = true; // first, assume we are
              for( Core c = 0; c < Grappa::locale_cores(); ++c ) {
                // then, update if we aren't.
                if( num_sent[ c ] < sent_messages_per_core ) done = false;
              }

              // enqueue the buffer to be processed and let a receiver do so
              DVLOG(4) << "Pushing buffer onto received list";
              Grappa::impl::global_rdma_aggregator.received_buffer_list_.push( b );
              remote_distributed_buffers_per_locale++;
              Grappa::yield();
            }
          }

          // wait for all completions.
          local_ce.wait();
        
          Grappa::barrier();
        

          BOOST_CHECK_EQUAL( local_count, expected_messages_per_core );
          CHECK_EQ( local_count, expected_messages_per_core ) << "Remote message distribution";
        } );

      double time = Grappa::walltime() - start;
      remote_distributed_messages_per_locale = expected_messages_per_locale;
      remote_distributed_messages_time = time;
      remote_distributed_messages_rate_per_locale = expected_messages_per_locale / time;
    }



  
  

    Grappa::Metrics::merge_and_print( std::cout );

  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
