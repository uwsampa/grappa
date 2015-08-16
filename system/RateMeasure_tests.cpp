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
