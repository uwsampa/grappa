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

#include "Grappa.hpp"
#include "CompletionEvent.hpp"

DECLARE_int64( aggregator_target_size );

BOOST_AUTO_TEST_SUITE( NTMessage_aggregator_tests );

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{

      { // multiple messages with explicit flush
        int x = 0;
        Grappa::CompletionEvent ce(3);
        Grappa::impl::global_rdma_aggregator.send_nt_message( 0, [&x,&ce] {
            x++;
            ce.complete();
          } );
        Grappa::impl::global_rdma_aggregator.send_nt_message( 0, [&x,&ce] {
            x++;
            ce.complete();
          } );
        Grappa::impl::global_rdma_aggregator.send_nt_message( 0, [&x,&ce] {
            x++;
            ce.complete();
          } );
        Grappa::impl::global_rdma_aggregator.flush_nt( 0 );
        ce.wait();
        BOOST_CHECK_EQUAL( x, 3 );
      }

      { // target size flush
        auto old_target_size = FLAGS_aggregator_target_size;
        FLAGS_aggregator_target_size = 24 * 3;

        int x = 0;
        Grappa::CompletionEvent ce(3);

        Grappa::impl::global_rdma_aggregator.send_nt_message( 0, [&x,&ce] {
            x++;
            ce.complete();
          } );
        Grappa::impl::global_rdma_aggregator.send_nt_message( 0, [&x,&ce] {
            x++;
            ce.complete();
          } );
        Grappa::impl::global_rdma_aggregator.send_nt_message( 0, [&x,&ce] {
            x++;
            ce.complete();
          } );
        ce.wait();
        BOOST_CHECK_EQUAL( x, 3 );

        ce.enroll();
        Grappa::impl::global_rdma_aggregator.send_nt_message( 0, [&x,&ce] {
            x++;
            ce.complete();
          } );
        Grappa::impl::global_rdma_aggregator.flush_nt( 0 );
        ce.wait();
        BOOST_CHECK_EQUAL( x, 4 );

        
        FLAGS_aggregator_target_size = old_target_size;
      }

      { // request/response with explicit flushes
        Grappa::CompletionEvent ce(1);
        Grappa::impl::global_rdma_aggregator.send_nt_message( 1, [&ce] {
            //LOG(INFO) << "Request message";
            Grappa::impl::global_rdma_aggregator.send_nt_message( 0, [&ce] {
                //LOG(INFO) << "Reply message";
                ce.complete();
              } );
            Grappa::impl::global_rdma_aggregator.flush_nt( 0 );
          } );
        Grappa::impl::global_rdma_aggregator.flush_nt( 1 );
        ce.wait();
        BOOST_CHECK_EQUAL( 1, 1 );
      }

      { // request/response with no flushes
        Grappa::CompletionEvent ce(1);
        Grappa::impl::global_rdma_aggregator.send_nt_message( 1, [&ce] {
            LOG(INFO) << "Request message";
            Grappa::impl::global_rdma_aggregator.send_nt_message( 0, [&ce] {
                LOG(INFO) << "Reply message";
                ce.complete();
              } );
          } );
        ce.wait();
        BOOST_CHECK_EQUAL( 1, 1 );
      }

    });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();

