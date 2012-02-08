#include <boost/test/unit_test.hpp>

#include <stdio.h>

#include "thread.h"


BOOST_AUTO_TEST_SUITE( thread_tests );

struct child_args {
    int value;
};
struct parent_args {
    thread* master;
    scheduler* sched;
};

void child_run( thread* me, void* args) {
    child_args* cargs = (child_args*) args;

    BOOST_CHECK_EQUAL( 101, cargs->value );
    //printf("%u is yielding\n", me->id);
//    thread_yield( me );
    BOOST_CHECK_EQUAL( 101, cargs->value );
    printf("%u is done\n", me->id);
    thread_exit( me, NULL );
}

void parent_run( thread* me, void* args) {
    parent_args* pargs = (parent_args*) args;

    child_args cargs1 = { 101 };
    child_args cargs2 = { 101 };
    
    thread* child1 = thread_spawn( pargs->master, pargs->sched, &child_run, &cargs1);
    thread* child2 = thread_spawn( pargs->master, pargs->sched, &child_run, &cargs2);

    //printf("%u is join 1\n", me->id);
    thread_join( me, child1 );
    cargs1.value = 91;
    //printf("%u is join 2\n", me->id);
    thread_join( me, child2 );
    cargs2.value = 92;

    thread_yield( me );

    destroy_thread(child1);
    destroy_thread(child2);

    thread_exit( me, NULL );
}

BOOST_AUTO_TEST_CASE( test_join ) {
       
    thread* master = thread_init();
    scheduler* sched = create_scheduler( master );
    
    parent_args pargs = { master, sched };
    thread* parent = thread_spawn( master, sched, &parent_run, &pargs);    
    run_all ( sched );


    destroy_scheduler(sched);
}



BOOST_AUTO_TEST_SUITE_END();
