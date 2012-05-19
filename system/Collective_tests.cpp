
#include <boost/test/unit_test.hpp>

#include "SoftXMT.hpp"
#include "Collective.hpp"
#include "ForkJoin.hpp"

BOOST_AUTO_TEST_SUITE( Collective_tests );

struct worker_args {
    int64_t add1Operand;
    int64_t max1Operand;

    int64_t add1Result;
    int64_t max1Result;
};

worker_args this_node_wargs;
const int64_t add_init = 0;
const int64_t max_init = -1000;

void worker_thread_f(Thread* me, void* args) {
    worker_args* wargs = (worker_args*) args;

    BOOST_MESSAGE( "worker " << SoftXMT_mynode() << " entering add reduce" );
    wargs->add1Result = SoftXMT_collective_reduce(COLL_ADD, 0, wargs->add1Operand, add_init);
    BOOST_MESSAGE( "worker " << SoftXMT_mynode() << " entering max reduce" );
    wargs->max1Result = SoftXMT_collective_reduce(COLL_MAX, 0, wargs->max1Operand, max_init);
    
    BOOST_MESSAGE( "worker " << SoftXMT_mynode() << " is finished" );
    SoftXMT_barrier_commsafe();
    BOOST_MESSAGE( "worker " << SoftXMT_mynode() << " exits finished barrier" );
}

void spawn_worker_am( worker_args* args, size_t size, void* payload, size_t payload_size ) {
   /* in general (for async am handling) this may need synchronization */
   memcpy(&this_node_wargs, args, size);
   BOOST_MESSAGE( "Remote is spawning worker " << SoftXMT_mynode() );
   SoftXMT_spawn(&worker_thread_f, &this_node_wargs); 
}

struct Data {
  int64_t x;
  double y;
  Data(int64_t x, double y): x(x), y(y) {}
  Data operator+(const Data& o) {
    return Data(x+o.x, y+o.y);
  }
};

LOOP_FUNCTION( all_reduce_test_func, nid ) {
  int64_t myval = 123;
  
  int64_t sum = SoftXMT_allreduce<int64_t,coll_add<int64_t>,0>(myval);
  BOOST_CHECK_EQUAL(sum, 123*SoftXMT_nodes());
}

void user_main( int * args ) 
{
    BOOST_MESSAGE( "Spawning user main Thread " << (void *) CURRENT_THREAD <<
            " " << CURRENT_THREAD <<
            " on node " << SoftXMT_mynode() );

    Node expectedNodes = 4; 
    BOOST_CHECK_EQUAL( expectedNodes, SoftXMT_nodes() );

    // spawn worker threads 
    Thread* worker_thread;
    worker_args wargss[expectedNodes];
    for (int nod = 0; nod<expectedNodes; nod++) {
        wargss[nod].add1Operand = nod-1;
        wargss[nod].max1Operand = -nod;
        if (nod == SoftXMT_mynode()) {
            BOOST_MESSAGE( "Spawing worker " << nod );
            worker_thread = SoftXMT_spawn( &worker_thread_f, &wargss[nod] );
        } else {
            BOOST_MESSAGE( "Spawing worker " << nod );
            SoftXMT_call_on( nod, &spawn_worker_am, &wargss[nod] );
        }
    }
    BOOST_MESSAGE( "user_main joins" );
    SoftXMT_join(worker_thread);//because 0 leads reduction this join will mean everything is done
    BOOST_MESSAGE( "user_main leaves join" );

    // calculate the reductions manually
    int64_t addres = add_init;
    int64_t maxres = max_init;
    for (int nod = 0; nod < expectedNodes; nod++) {
        addres += wargss[nod].add1Operand; 
        maxres = (wargss[nod].max1Operand > maxres) ? 
                        wargss[nod].max1Operand 
                      : maxres;
    }
    BOOST_CHECK_EQUAL ( addres, wargss[0].add1Result );
    BOOST_CHECK_EQUAL ( maxres, wargss[0].max1Result );

  all_reduce_test_func f;
  fork_join_custom(&f);

    SoftXMT_signal_done();
}

BOOST_AUTO_TEST_CASE( test1 ) {

  SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  SoftXMT_activate();

  SoftXMT_run_user_main( &user_main, (int*)NULL );
  BOOST_CHECK( SoftXMT_done() == true );

  SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

