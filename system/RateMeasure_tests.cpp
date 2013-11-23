// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

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

GRAPPA_DEFINE_STAT(SimpleStatistic<double>, sing_runtime, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<double>, sing_rate, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<double>, stream_runtime, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<double>, stream_rate, 0);

BOOST_AUTO_TEST_SUITE( RateMeasure_tests );


BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{

    int num_tasks = 256;

    Grappa::Statistics::reset_all_cores();

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
        VLOG(3) << Grappa::mycore() << "sends to " << addr.node();
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
  
    Grappa::Statistics::merge_and_print();
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
