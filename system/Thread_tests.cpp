// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/test/unit_test.hpp>

#include "tasks/Thread.hpp"
#include "tasks/BasicScheduler.hpp"
#include <glog/logging.h>

#include <time.h>

// FIXME: Thread_tests depends on Thread->PerformanceTools->StateTimer->SoftXMT
//        Thread_tests is supposed to be independent of a Grappa runtime
//        To fix this spurious dependence with minimal changes, there could be
//        a compiled-with-grappa macro variable that wraps usage of StateTimer in Thread.*pp

BOOST_AUTO_TEST_SUITE( Thread_tests );

void thread_f1( Thread* me, void* args ) {
    BOOST_MESSAGE( "i am Thread f1" );
}

void thread_f2( Thread* me, void* args ) {
    BOOST_MESSAGE( "i am Thread f2" );
}

BOOST_AUTO_TEST_CASE( test_spawn_and_finish ) {
    Thread* master = thread_init();

    BasicScheduler* sched = new BasicScheduler( master ); 

    Thread* t1 = thread_spawn( master, sched, thread_f1, NULL );
    Thread* t2 = thread_spawn( master, sched, thread_f2, NULL );
    sched->ready( t1 );
    sched->ready( t2 );

    BOOST_MESSAGE( "call run" );
    sched->run( );
    BOOST_MESSAGE( "finished" );
}


struct sched_arg {
    Scheduler * sched;
    Thread * t1;
};

void thread_f1_2( Thread* me, void* args ) {
    sched_arg * sargs = (sched_arg *) args;
    
    BOOST_MESSAGE( "f1 yields" );
    sargs->sched->thread_yield( );
    BOOST_MESSAGE( "f1 runs again" );
}

void thread_f2_2( Thread* me, void* args ) {
    sched_arg * sargs = (sched_arg *) args;
    
    BOOST_MESSAGE( "f2 yields" );
    sargs->sched->thread_yield( );
    BOOST_MESSAGE( "f2 runs again" );
    
}

BOOST_AUTO_TEST_CASE( test_yield ) {
    Thread* master = thread_init();

    BasicScheduler* sched = new BasicScheduler( master ); 
    
    sched_arg arg = { sched, NULL };
    Thread* t1 = thread_spawn( master, sched, thread_f1_2, &arg );
    Thread* t2 = thread_spawn( master, sched, thread_f2_2, &arg );
    sched->ready( t1 );
    sched->ready( t2 );

    BOOST_MESSAGE( "call run" );
    sched->run( );
    BOOST_MESSAGE( "finished" );
}


void thread_f1_3( Thread* me, void* args ) {
    sched_arg * sargs = (sched_arg *) args;
    
    BOOST_MESSAGE( "f1 suspends" );
    sargs->sched->thread_suspend( ); // depend on this happens first for this test
    BOOST_MESSAGE( "f1 runs again" );
}

void thread_f2_3( Thread* me, void* args ) {
    sched_arg * sargs = (sched_arg *) args;
    
    BOOST_MESSAGE( "f2 will try wake" );
    sargs->sched->thread_wake( sargs->t1 );
    BOOST_MESSAGE( "f2 tried wake f1" );
    
}

BOOST_AUTO_TEST_CASE( test_suspend ) {
    Thread* master = thread_init();

    BasicScheduler* sched = new BasicScheduler( master ); 
    
    sched_arg arg = { sched, NULL };
    Thread* t1 = thread_spawn( master, sched, thread_f1_3, &arg );
    sched_arg arg2 = { sched, t1 };
    Thread* t2 = thread_spawn( master, sched, thread_f2_3, &arg2 );
    sched->ready( t1 );
    sched->ready( t2 );

    BOOST_MESSAGE( "call run" );
    sched->run( );
    BOOST_MESSAGE( "finished" );
}

struct time_args {
    BasicScheduler* sched;
    uint64_t num;
};

void threadf_yielding( Thread * me, void* args ) {
    time_args* targs = (time_args *) args;
    BasicScheduler * sched = targs->sched;
    uint64_t num = targs->num;
    for (int i=0; i<num; i++) {
        sched->thread_yield();
    }
    BOOST_MESSAGE( me->id << " done" );
}

double calcInterval( struct timespec start, struct timespec end ) {
    const uint64_t BILLION_ = 1000000000;
    return (end.tv_sec + (((double)end.tv_nsec)/BILLION_)) - (start.tv_sec + (((double)start.tv_nsec)/BILLION_));
}

BOOST_AUTO_TEST_CASE( benchmark_self_yield ) {
    Thread* master = thread_init();

    BasicScheduler* sched = new BasicScheduler( master ); 
    
    const uint64_t num_yield = 20000000;
    time_args arg = { sched, num_yield };
    Thread* t1 = thread_spawn( master, sched, threadf_yielding, &arg );
    sched->ready( t1 );
    
    struct timespec start, end;
    BOOST_MESSAGE( "call run" );
    clock_gettime( CLOCK_MONOTONIC, &start );
    sched->run( );
    clock_gettime( CLOCK_MONOTONIC, &end );
    BOOST_MESSAGE( "finished" );
    
    double runtime_s = calcInterval( start, end );
    double ns_per_yield = (runtime_s*1000000000) / num_yield;
    BOOST_MESSAGE( ns_per_yield << " ns per yield (avg)" );
}

BOOST_AUTO_TEST_CASE( benchmark_two_yield ) {
    Thread* master = thread_init();

    BasicScheduler* sched = new BasicScheduler( master ); 
    
    const uint64_t num_yield = 20000000;
    const uint64_t num_coro = 2;
    time_args arg = { sched, num_yield };
    for (uint64_t th; th<num_coro; th++) {
        Thread* t1 = thread_spawn( master, sched, threadf_yielding, &arg );
        sched->ready( t1 );
    }
     
    struct timespec start, end;
    BOOST_MESSAGE( "call run" );
    clock_gettime( CLOCK_MONOTONIC, &start );
    sched->run( );
    clock_gettime( CLOCK_MONOTONIC, &end );
    BOOST_MESSAGE( "finished" );
    
    double runtime_s = calcInterval( start, end );
    double ns_per_yield = (runtime_s*1000000000) / (num_yield*num_coro); 
    BOOST_MESSAGE( ns_per_yield << " ns per yield (avg)" );
}


BOOST_AUTO_TEST_SUITE_END();
