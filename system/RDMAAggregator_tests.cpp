
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.


#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "RDMAAggregator.hpp"
#include "Message.hpp"
#include "CompletionEvent.hpp"
#include "Statistics.hpp"
#include "ParallelLoop.hpp"
#include "ReuseMessage.hpp"

DEFINE_int64( iterations_per_core, 1 << 24, "Number of messages sent per core" );

BOOST_AUTO_TEST_SUITE( RDMAAggregator_tests );


int count = 0;
int count2 = 0;

Grappa::CompletionEvent local_ce;
size_t local_count = 0;

GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, local_messages_per_locale, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<double>, local_messages_time, 0.0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<double>, local_messages_rate_per_locale, 0.0 );


void user_main( void * args ) {
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
      
    double start = Grappa_walltime();
    
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
    
    double time = Grappa_walltime() - start;
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
  {
    LOG(INFO) << "Testing local message delivery";
    const int64_t expected_messages_per_core = Grappa::locale_cores() * FLAGS_iterations_per_core;
    const int64_t expected_messages_per_locale = expected_messages_per_core * Grappa::locale_cores();

    struct LocalDelivery {
      Core source_core;
      Core dest_core;
      void operator()() { 
        local_count++; 
        local_ce.complete(); 
      }
    };
    
    double start = Grappa_walltime();

    Grappa::on_all_cores( [expected_messages_per_core] {
        // prepare pool of 1024 messages
        Grappa::ReuseMessageList< LocalDelivery > msgs( 1024 );
        msgs.activate(); // allocate messages

        { 
          local_ce.enroll( expected_messages_per_core );
          LOG(INFO) << "Core " << Grappa::mycore() << " waiting for " << local_ce.get_count() << " updates.";

          Grappa_barrier_suspending();

          for( int i = 0; i < FLAGS_iterations_per_core; ++i ) {
            for( int locale_core = 0; locale_core < Grappa::locale_cores(); ++locale_core ) {
              Core mycore = Grappa::mycore();
              Core dest_core = locale_core + Grappa::mylocale() * Grappa::locale_cores();
              msgs.with_message( [mycore, dest_core] ( Grappa::ReuseMessage<LocalDelivery> * m ) {
                  (*m)->source_core = mycore;
                  (*m)->dest_core = dest_core;
                  CHECK_EQ( Grappa::locale_of( mycore ), Grappa::locale_of( dest_core ) );
                  m->enqueue( dest_core );
                } );
            }
          }
          
          local_ce.wait();

          Grappa_barrier_suspending();

          BOOST_CHECK_EQUAL( local_count, expected_messages_per_core );
          CHECK_EQ( local_count, expected_messages_per_core ) << "Local message delivery";
        }
        
        msgs.finish(); // clean up messages
      } );

    double time = Grappa_walltime() - start;
    local_messages_per_locale = expected_messages_per_locale;
    local_messages_time = time;
    local_messages_rate_per_locale = expected_messages_per_locale / time;
  }

  // test aggregation

  // test message distribution



  
  

  Grappa::Statistics::merge_and_print();

}



BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
               &(boost::unit_test::framework::master_test_suite().argv),
               (1L << 20) );

  Grappa::impl::global_rdma_aggregator.init();
  Grappa_activate();

  Grappa_run_user_main( &user_main, (void*)NULL );

  BOOST_CHECK_EQUAL( count, 6 );

  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

