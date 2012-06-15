#include <boost/test/unit_test.hpp>

#include "SoftXMT.hpp"
#include "Delegate.hpp"
#include "Tasking.hpp"
#include "ParallelLoop.hpp"



BOOST_AUTO_TEST_SUITE( ParallelLoop_tests );

int64_t ind_local_count = 0;
int64_t dub_local_count = 0;

struct array_args {
    int64_t * array;
};

void ind_array( int64_t i, array_args * a ) {
    ind_local_count++;
    SoftXMT_delegate_write_word( make_global( a->array+i, 0 ), i ); // +i needs to be INSIDE make_global here because we know it is all on node0
}

void dub_array( int64_t i, array_args * a ) {
    dub_local_count++;
    GlobalAddress<int64_t> addr = make_global( a->array+i, 0);
    CHECK( (int64_t)addr.pointer() > 0x1000 ) << "calc addr:"<<addr<<"\n a="<<(void*)a<<"\n iter="<<i<<"\n array:"<<(void*)a->array<<" \narray+i:"<<a->array+i;
    int64_t x =  SoftXMT_delegate_read_word( addr );
    SoftXMT_delegate_write_word( addr, 2*x ); //here!
}

struct count_args {
    GlobalAddress<int64_t> ind;
    GlobalAddress<int64_t> dub;

    count_args() {} // Future templates complain without this defualt

    count_args(int64_t * ind, int64_t * dub)
    : ind( make_global(ind) )
    , dub( make_global(dub) ){}
};


void countUp( int64_t i, count_args * args ) {
    int64_t ind_contrib = SoftXMT_delegate_read_word( make_global( &ind_local_count, i ) );
    int64_t dub_contrib = SoftXMT_delegate_read_word( make_global( &dub_local_count, i ) );

    SoftXMT_delegate_write_word( args->ind + i, ind_contrib );
    SoftXMT_delegate_write_word( args->dub + i, dub_contrib );
}

struct user_main_args {
};

void user_main( user_main_args * args ) 
{
    int64_t length1 = 1024;
    int64_t * array1 = new int64_t[length1];
    array_args aa = { array1 };
    BOOST_MESSAGE( "&array1="<<array1<< "&aa="<<&aa );
    
    // a[i] = i
    parallel_loop( 0, length1, &ind_array, aa );
    // check
    for (int i=0; i<length1; i++) {
        BOOST_CHECK_EQUAL( array1[i], i );
    }

    // a[i] = a[i]*2
    parallel_loop( 0, length1, &dub_array, aa ); 
    // check
    for (int i=0; i<length1; i++) {
        BOOST_CHECK_EQUAL( array1[i], 2*i );
    }
    delete array1;

    
    // stats: count up how many iterations each Node ran
    int64_t * ind_counts = new int64_t[SoftXMT_nodes()];
    int64_t * dub_counts = new int64_t[SoftXMT_nodes()];
    count_args cargs ( ind_counts, dub_counts );
    parallel_loop( 0, SoftXMT_nodes(), &countUp, cargs ); 

    for (Node i=0; i<SoftXMT_nodes(); i++) {
        BOOST_MESSAGE( "node" << i << ":"
                       << ind_counts[i] << " ind, "
                       << dub_counts[i] << " dub" );
    }
    delete ind_counts;
    delete dub_counts;

    BOOST_MESSAGE( "user main is exiting" );
    SoftXMT_end_tasks();
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

