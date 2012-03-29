#include <boost/test/unit_test.hpp>

#include "Thread.hpp"
#include "BasicScheduler.hpp"
#include <glog/logging.h>

BOOST_AUTO_TEST_SUITE( Thread_tests );

void thread_f1( thread* me, void* args ) {
    BOOST_MESSAGE( "i am thread f1" );
}

void thread_f2( thread* me, void* args ) {
    BOOST_MESSAGE( "i am thread f2" );
}

BOOST_AUTO_TEST_CASE( test_spawn_and_finish ) {
    thread* master = thread_init();

    BasicScheduler* sched = new BasicScheduler( master ); 

    thread* t1 = thread_spawn( master, sched, thread_f1, NULL );
    thread* t2 = thread_spawn( master, sched, thread_f2, NULL );
    sched->ready( t1 );
    sched->ready( t2 );

    BOOST_MESSAGE( "call run" );
    sched->run( );
    BOOST_MESSAGE( "finished" );
}


struct sched_arg {
    Scheduler * sched;
    thread * t1;
};

void thread_f1_2( thread* me, void* args ) {
    sched_arg * sargs = (sched_arg *) args;
    
    BOOST_MESSAGE( "f1 yields" );
    sargs->sched->thread_yield( );
    BOOST_MESSAGE( "f1 runs again" );
}

void thread_f2_2( thread* me, void* args ) {
    sched_arg * sargs = (sched_arg *) args;
    
    BOOST_MESSAGE( "f2 yields" );
    sargs->sched->thread_yield( );
    BOOST_MESSAGE( "f2 runs again" );
    
}

BOOST_AUTO_TEST_CASE( test_yield ) {
    thread* master = thread_init();

    BasicScheduler* sched = new BasicScheduler( master ); 
    
    sched_arg arg = { sched, NULL };
    thread* t1 = thread_spawn( master, sched, thread_f1_2, &arg );
    thread* t2 = thread_spawn( master, sched, thread_f2_2, &arg );
    sched->ready( t1 );
    sched->ready( t2 );

    BOOST_MESSAGE( "call run" );
    sched->run( );
    BOOST_MESSAGE( "finished" );
}


void thread_f1_3( thread* me, void* args ) {
    sched_arg * sargs = (sched_arg *) args;
    
    BOOST_MESSAGE( "f1 suspends" );
    sargs->sched->thread_suspend( ); // depend on this happens first for this test
    BOOST_MESSAGE( "f1 runs again" );
}

void thread_f2_3( thread* me, void* args ) {
    sched_arg * sargs = (sched_arg *) args;
    
    BOOST_MESSAGE( "f2 will try wake" );
    sargs->sched->thread_wake( sargs->t1 );
    BOOST_MESSAGE( "f2 tried wake f1" );
    
}

BOOST_AUTO_TEST_CASE( test_suspend ) {
    thread* master = thread_init();

    BasicScheduler* sched = new BasicScheduler( master ); 
    
    sched_arg arg = { sched, NULL };
    thread* t1 = thread_spawn( master, sched, thread_f1_3, &arg );
    sched_arg arg2 = { sched, t1 };
    thread* t2 = thread_spawn( master, sched, thread_f2_3, &arg2 );
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

void threadf_yielding( thread * me, void* args ) {
    time_args* targs = (time_args *) args;
    BasicScheduler * sched = targs->sched;
    uint64_t num = targs->num;
    for (int i=0; i<num; i++) {
        sched->thread_yield();
    }
    BOOST_MESSAGE( me->id << " done" );
}

#define rdtscll(val) do { \
    unsigned int __a,__d; \
    asm volatile("rdtsc" : "=a" (__a), "=d" (__d)); \
    (val) = ((unsigned long)__a) | (((unsigned long)__d)<<32); \
  } while(0)

BOOST_AUTO_TEST_CASE( benchmark_self_yield ) {
    thread* master = thread_init();

    BasicScheduler* sched = new BasicScheduler( master ); 
    
    const uint64_t num_yield = 20000000;
    time_args arg = { sched, num_yield };
    thread* t1 = thread_spawn( master, sched, threadf_yielding, &arg );
    sched->ready( t1 );
    
    uint64_t start, end;
    BOOST_MESSAGE( "call run" );
    rdtscll(start);
    sched->run( );
    rdtscll(end);
    BOOST_MESSAGE( "finished" );
    
    double runtime_ns = (end - start) / 2.66;
    double ns_per_yield = runtime_ns / num_yield;
    BOOST_MESSAGE( ns_per_yield << " ns per yield (avg)" );
}

BOOST_AUTO_TEST_CASE( benchmark_two_yield ) {
    thread* master = thread_init();

    BasicScheduler* sched = new BasicScheduler( master ); 
    
    const uint64_t num_yield = 20000000;
    time_args arg = { sched, num_yield };
    thread* t1 = thread_spawn( master, sched, threadf_yielding, &arg );
    thread* t2 = thread_spawn( master, sched, threadf_yielding, &arg );
    sched->ready( t1 );
    sched->ready( t2 );
    
    uint64_t start, end;
    BOOST_MESSAGE( "call run" );
    rdtscll(start);
    sched->run( );
    rdtscll(end);
    BOOST_MESSAGE( "finished" );
    
    double runtime_ns = (end - start) / 2.66;
    double ns_per_yield = runtime_ns / (num_yield*2); // two threads
    BOOST_MESSAGE( ns_per_yield << " ns per yield (avg)" );
}

BOOST_AUTO_TEST_SUITE_END();
