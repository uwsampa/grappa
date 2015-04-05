////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
////////////////////////////////////////////////////////////////////////

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "CompletionEvent.hpp"

DECLARE_int64( aggregator_target_size );

BOOST_AUTO_TEST_SUITE( NTMessage_aggregator_tests );

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{

      {
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

      {
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

      {
        Grappa::CompletionEvent ce(1);
        Grappa::impl::global_rdma_aggregator.send_nt_message( 1, [&ce] {
            LOG(INFO) << "Request message";
            Grappa::impl::global_rdma_aggregator.send_nt_message( 0, [&ce] {
                LOG(INFO) << "Reply message";
                ce.complete();
              } );
            Grappa::impl::global_rdma_aggregator.flush_nt( 0 );
          } );
        Grappa::impl::global_rdma_aggregator.flush_nt( 1 );
        ce.wait();
        BOOST_CHECK_EQUAL( 1, 1 );
      }
    });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();

