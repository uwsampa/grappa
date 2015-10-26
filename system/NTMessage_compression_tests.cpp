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

#define ENABLE_NT_MESSAGE
#define USE_NT_OPS
#include "Grappa.hpp"

DEFINE_int64( message_count, 16, "Number of messages to send in test" );
DECLARE_int64( aggregator_target_size );

Grappa::CompletionEvent ce;

BOOST_AUTO_TEST_SUITE( NTMessage_compression_tests );

size_t buffer_size_for( Core c ) {
  const auto & buf = Grappa::impl::global_rdma_aggregator.ntbuffers_[c];
  if( buf.size() ) {
    return buf.size() - buf.initial_offset();
  } else {
    return 0;
  }
}

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{

    // no-address messages to test only handler compression
    {
      // ensure target size is appropriate for this test.  we want to
      // be able to write a bunch of messages into the buffer without
      // it filling up and sending by itself, so we can measure the
      // stored size.
      if( FLAGS_aggregator_target_size < FLAGS_message_count * 16 ) {
        LOG(WARNING) << "With specified target size this test may execute handlers before we expect";
      }
        
      // start with an empty buffer
      Grappa::impl::global_rdma_aggregator.flush_nt( 0 );
      BOOST_CHECK_EQUAL( true, Grappa::impl::global_rdma_aggregator.ntbuffers_->empty() );

      // send a bunch of messages to core 0
      ce.enroll( FLAGS_message_count );
      for( int i = 0; i < FLAGS_message_count; ++i ) {
        Grappa::impl::global_rdma_aggregator.send_nt_message( 0, [] {
          ce.complete();
        });
      }

      // Now the core 0 buffer should have a bunch of messages.
      // The first test currently passes; adding compression should make the second one pass instead
      BOOST_CHECK_EQUAL( FLAGS_message_count * 16, buffer_size_for(0) );
      // BOOST_CHECK_EQUAL( 8 + FLAGS_message_count * 8, buffer_size_for(0) );
      
      // force messages to be sent, and wait for them to be received
      Grappa::impl::global_rdma_aggregator.flush_nt( 0 );
      ce.wait();
      
      // everything has executed, buffer is empty
      BOOST_CHECK_EQUAL( 0, Grappa::impl::global_rdma_aggregator.ntbuffers_->size() );
    }
    


    // // one-address messages to test handler compression plus contiguous address coalescing
    // // (This API is work-in-progress and doesn't work yet, so it's commented out)
    // if(0) {
    //   // declare destination array
    //   static int64_t array[1024];
      
    //   // ensure target size is appropriate for this test.  we want to
    //   // be able to write a bunch of messages into the buffer without
    //   // it filling up and sending by itself, so we can measure the
    //   // stored size.
    //   if( FLAGS_aggregator_target_size < FLAGS_message_count * 16 ) {
    //     LOG(WARNING) << "With specified target size this test may execute handlers before we expect";
    //   }
        
    //   // start with an empty buffer
    //   Grappa::impl::global_rdma_aggregator.flush_nt( 0 );
    //   BOOST_CHECK_EQUAL( true, Grappa::impl::global_rdma_aggregator.ntbuffers_->empty() );

    //   // send a bunch of messages to core 0
    //   ce.enroll( FLAGS_message_count );
    //   for( int64_t i = 0; i < FLAGS_message_count; ++i ) {
    //     Grappa::impl::global_rdma_aggregator.send_nt_message( make_global( &array[i], 0 ), [i] ( int64_t& val ) {
    //       val = i;
    //       ce.complete();
    //     });
    //   }

    //   // Now the core 0 buffer should have a bunch of messages.
    //   // The first test currently passes; adding compression should make the second one pass instead
    //   BOOST_CHECK_EQUAL( FLAGS_message_count * 16, buffer_size_for(0) );
    //   // BOOST_CHECK_EQUAL( 8 + FLAGS_message_count * 8, buffer_size_for(0) );
      
    //   // force messages to be sent, and wait for them to be received
    //   Grappa::impl::global_rdma_aggregator.flush_nt( 0 );
    //   ce.wait();
      
    //   // everything has executed, buffer is empty
    //   BOOST_CHECK_EQUAL( 0, Grappa::impl::global_rdma_aggregator.ntbuffers_->size() );
    // }
    
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
