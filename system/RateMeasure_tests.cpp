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
#include "Tasking.hpp"
#include "Delegate.hpp"
#include "common.hpp"
#include "Collective.hpp"

// Tests to measure the delegate op rates
// - Single location: every Core reads a single location on a single other Core on another machine
// - Stream location: every Core reads contiguous locations on a single other Core on another machine
// Must set the --num_places value to the number of network interfaces to ensure all ops use the network

const uint64_t data_size = 1<<10;
int64_t some_data[data_size];

DEFINE_int64( num_places, 2, "Number of network interfaces" );

#include <sys/time.h>
double wctime() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec + 1E-6 * tv.tv_usec);
}

GRAPPA_DEFINE_METRIC(SimpleMetric<double>, sing_runtime, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, sing_rate, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, stream_runtime, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, stream_rate, 0);

BOOST_AUTO_TEST_SUITE( RateMeasure_tests );


BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{

    int num_tasks = 256;

    Grappa::Metrics::reset_all_cores();

    {
      double start, end;
      BOOST_MESSAGE( "single_location..." );
      start = wctime();
    
      Grappa::on_all_cores([]{
        int64_t nodes_per_place = Grappa::cores() / FLAGS_num_places;
        Core dest = (Grappa::mycore()+nodes_per_place)%Grappa::cores();
        GlobalAddress<int64_t> addr = make_global( &some_data[0], dest);
        VLOG(3) << Grappa::mycore() << "sends to " << dest;
        for ( uint64_t i=0; i<data_size; i++ ) {
            Grappa::delegate::read( addr );
        }
      });
    
      end = wctime();
      sing_runtime = end - start;
      sing_rate = (data_size*num_tasks*Grappa::cores())/sing_runtime;
      BOOST_MESSAGE( "single location: " << sing_runtime <<" s, " 
                      << sing_rate << " reads/sec" );
    }

    {
      double start, end;
      BOOST_MESSAGE( "stream_location..." );
      start = wctime();
  
      Grappa::on_all_cores([]{
        int64_t nodes_per_place = Grappa::cores() / FLAGS_num_places;
        Core dest = (Grappa::mycore()+nodes_per_place)%Grappa::cores();
        GlobalAddress<int64_t> addr = make_global( &some_data[0], dest);
        VLOG(3) << Grappa::mycore() << "sends to " << addr.core();
        for ( uint64_t i=0; i<data_size; i++ ) {
            Grappa::delegate::read( addr + i );
        }
      });
  
      end = wctime();
      stream_runtime = end - start;
      stream_rate = (data_size*num_tasks*Grappa::cores())/stream_runtime;
      BOOST_MESSAGE( "stream location: " << stream_runtime <<" s, " 
                      << stream_rate << " reads/sec" );
    }
  
    BOOST_MESSAGE( "User main exiting" );
  
    Grappa::Metrics::merge_and_print();
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
