// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/test/unit_test.hpp>

#include "tasks/DictOut.hpp"

#include "Grappa.hpp"
#include "Tasking.hpp"
#include "Delegate.hpp"
#include "ForkJoin.hpp"
#include "common.hpp"

// Tests to measure the delegate op rates
// - Single location: every Node reads a single location on a single other Node on another machine
// - Stream location: every Node reads contiguous locations on a single other Node on another machine
// Must set the --num_places value to the number of network interfaces to ensure all ops use the network

#define BILLION 1000000000
#define MILLION 1000000

const uint64_t data_size = 1<<10;
int64_t some_data[data_size];

DEFINE_int64( num_places, 2, "Number of network interfaces" );

#include <sys/time.h>
double wctime() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec + 1E-6 * tv.tv_usec);
}

BOOST_AUTO_TEST_SUITE( RateMeasure_tests );

struct user_main_args {
};

struct single_location_func : public ForkJoinIteration {
    void operator()(int64_t i) const {
        int64_t nodes_per_place = Grappa_nodes() / FLAGS_num_places;
        Node dest = (Grappa_mynode()+nodes_per_place)%Grappa_nodes();
        GlobalAddress<int64_t> addr = make_global( &some_data[0], dest);
        VLOG(3) << Grappa_mynode() << "sends to " << dest;
        for ( uint64_t i=0; i<data_size; i++ ) {
            Grappa_delegate_read_word( addr );
        }
    }
};

struct stream_location_func : public ForkJoinIteration {
    void operator()(int64_t i) const {
        int64_t nodes_per_place = Grappa_nodes() / FLAGS_num_places;
        Node dest = (Grappa_mynode()+nodes_per_place)%Grappa_nodes();
        GlobalAddress<int64_t> addr = make_global( &some_data[0], dest);
        VLOG(3) << Grappa_mynode() << "sends to " << addr.node();
        for ( uint64_t i=0; i<data_size; i++ ) {
            Grappa_delegate_read_word( addr + i );
        }
    }
};

void user_main( user_main_args * args ) {
 
    int num_tasks = 256;

    Grappa_reset_stats_all_nodes();
  
    double sing_runtime, sing_rate, stream_runtime, stream_rate; 
    {
    double start, end;
    single_location_func f;
    BOOST_MESSAGE( "single_location..." );
    start = wctime();
    fork_join( &f, 0, (num_tasks*Grappa_nodes())-1 );
    end = wctime();
    sing_runtime = end - start;
    sing_rate = (data_size*num_tasks*Grappa_nodes())/sing_runtime;
    BOOST_MESSAGE( "single location: " << sing_runtime <<" s, " 
                    << sing_rate << " reads/sec" );
    }

    {
    double start, end;
    stream_location_func f;
    BOOST_MESSAGE( "stream_location..." );
    start = wctime();
    fork_join( &f, 0, (num_tasks*Grappa_nodes())-1 );
    end = wctime();
    stream_runtime = end - start;
    stream_rate = (data_size*num_tasks*Grappa_nodes())/stream_runtime;
    BOOST_MESSAGE( "stream location: " << stream_runtime <<" s, " 
                    << stream_rate << " reads/sec" );
    }

    DictOut d;
    d.add( "single_location_runtime_s", sing_runtime );
    d.add( "single_location_rate", sing_rate );
    d.add( "stream_location_runtime_s", stream_runtime );
    d.add( "stream_location_rate", stream_rate );
    BOOST_MESSAGE( "RateMeasure_tests: " << d.toString() );
    
    BOOST_MESSAGE( "User main exiting" );
    //Grappa_dump_stats_all_nodes();
    Grappa_merge_and_dump_stats();
}

BOOST_AUTO_TEST_CASE( test1 ) {

    Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
            &(boost::unit_test::framework::master_test_suite().argv) );

    Grappa_activate();

    user_main_args uargs;

    DVLOG(1) << "Spawning user main Thread....";
    Grappa_run_user_main( &user_main, &uargs );
    VLOG(5) << "run_user_main returned";
    CHECK( Grappa_done() );

    Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();
