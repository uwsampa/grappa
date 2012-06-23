#include <boost/test/unit_test.hpp>

#include "tasks/DictOut.hpp"

#include "SoftXMT.hpp"
#include "Tasking.hpp"
#include "Delegate.hpp"
#include "ForkJoin.hpp"
#include "common.hpp"

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
        int64_t nodes_per_place = SoftXMT_nodes() / FLAGS_num_places;
        Node dest = (SoftXMT_mynode()+nodes_per_place)%SoftXMT_nodes();
        GlobalAddress<int64_t> addr = make_global( &some_data[0], dest);
        VLOG(3) << SoftXMT_mynode() << "sends to " << dest;
        for ( uint64_t i=0; i<data_size; i++ ) {
            SoftXMT_delegate_read_word( addr );
        }
    }
};

struct stream_location_func : public ForkJoinIteration {
    void operator()(int64_t i) const {
        int64_t nodes_per_place = SoftXMT_nodes() / FLAGS_num_places;
        Node dest = (SoftXMT_mynode()+nodes_per_place)%SoftXMT_nodes();
        GlobalAddress<int64_t> addr = make_global( &some_data[0], dest);
        VLOG(3) << SoftXMT_mynode() << "sends to " << addr.node();
        for ( uint64_t i=0; i<data_size; i++ ) {
            SoftXMT_delegate_read_word( addr + i );
        }
    }
};

void user_main( user_main_args * args ) {
 
    int num_tasks = 256;

    SoftXMT_reset_stats_all_nodes();
  
    double sing_runtime, sing_rate, stream_runtime, stream_rate; 
    {
    double start, end;
    single_location_func f;
    BOOST_MESSAGE( "single_location..." );
    start = wctime();
    fork_join( &f, 0, (num_tasks*SoftXMT_nodes())-1 );
    end = wctime();
    sing_runtime = end - start;
    sing_rate = (data_size*num_tasks*SoftXMT_nodes())/sing_runtime;
    BOOST_MESSAGE( "single location: " << sing_runtime <<" s, " 
                    << sing_rate << " reads/sec" );
    }

    {
    double start, end;
    stream_location_func f;
    BOOST_MESSAGE( "stream_location..." );
    start = wctime();
    fork_join( &f, 0, (num_tasks*SoftXMT_nodes())-1 );
    end = wctime();
    stream_runtime = end - start;
    stream_rate = (data_size*num_tasks*SoftXMT_nodes())/stream_runtime;
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
    SoftXMT_dump_stats_all_nodes();
}

BOOST_AUTO_TEST_CASE( test1 ) {

    SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
            &(boost::unit_test::framework::master_test_suite().argv) );

    SoftXMT_activate();

    user_main_args uargs;

    DVLOG(1) << "Spawning user main Thread....";
    SoftXMT_run_user_main( &user_main, &uargs );
    VLOG(5) << "run_user_main returned";
    CHECK( SoftXMT_done() );

    SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();
