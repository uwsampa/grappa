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
#include "Delegate.hpp"
#include "Tasking.hpp"

// This is a deprecated test.
// - Currently is not updated to latest user_main interface
// - hard to force particular stealing behavior makes the test unreliable
// May be able to be turned into a robust test with more thought.
// One thing is that FLAGS_num_starting_workers should be CHECK'ed to ensure
// stealing will occur.

// tasks_per_node*nodes tasks are spawned; all but 4 meant to be stolen,
// chunksize is 4 to ensure every node gets copy of indices 0,1,2,3.
// A index task on each node must enter the multibarrier to continue
BOOST_AUTO_TEST_SUITE( Stealing_tests );

int tasks_per_node = 4;
int num_tasks;
int64_t num_finished=0;
int64_t num_stolen_started = 0;
int64_t vals[4] = {0};
Worker * threads[4]={NULL};
Worker * dummy_thr=NULL;
bool isWoken[4] = {false};
bool isActuallyAsleep[4] = {false};

struct task1_arg {
    int num;
    Worker * parent;
};

struct wake_arg {
    Worker * wakee;
};

void wake_f( wake_arg * args, size_t size, void * payload, size_t payload_size ) {
    Grappa::wake( args->wakee );
}

struct wakeindex_args {
    int index;
};

void wakeindex_f( wakeindex_args * args, size_t size, void * payload, size_t payload_size ) {
    isWoken[args->index] = true;
    if (isActuallyAsleep[args->index]) {
        Grappa::wake( threads[args->index] );
    }
}

void multiBarrier( int index ) {
    // increment the val on Core 0
    GlobalAddress<int64_t> vals_addr = GlobalAddress<int64_t>::TwoDimensional( &vals[index], 0 );
    int64_t result = Grappa_delegate_fetch_and_add_word( vals_addr, 1 );
    if ( result < Grappa::cores()-1 ) {
        // I not last so suspend
        BOOST_MESSAGE( index << " suspended index:" << result);
        isActuallyAsleep[index] = true;
        if ( !isWoken[index] ) {
            Grappa::suspend( );
        }
        BOOST_MESSAGE( index << " wake from barrier");
    } else if ( result == Grappa::cores()-1 ) {
        BOOST_MESSAGE( index << " is last and sending");
        // I am last so wake other
        wakeindex_args warg = { index };
        for (Core no = 1; no < Grappa::cores(); no++ ) {
            Core dest = (Grappa::mycore() + no) % Grappa::cores();
            //Grappa_call_on( dest, &wakeindex_f, &warg );
            send_heap_message( dest, [index] {
                isWoken[index] = true;
                if (isActuallyAsleep[index]) {
                  Grappa::wake( threads[index] );
                }
              } );
        }
    } else {
        BOOST_MESSAGE( result << " == " << Grappa::cores()-1 );
        BOOST_CHECK( result == Grappa::cores()-1 );
    }
}

void task_local( task1_arg * arg ) {
    int mynum = arg->num;
    Worker * parent = arg->parent;
   
    // this task should not have been stolen and running on 0 
    BOOST_CHECK_EQUAL( 0, Grappa::mycore() );

    BOOST_MESSAGE( Grappa::current_worker() << " with task(local) " << mynum << " about to enter multi barrier" );
    threads[mynum] = Grappa::current_worker(); // store my Worker ptr in local global array
    multiBarrier( mynum );
    
    // increment the global counter
    GlobalAddress<int64_t> nf_addr = GlobalAddress<int64_t>::TwoDimensional( &num_finished, 0 );
    int64_t result = Grappa_delegate_fetch_and_add_word( nf_addr, 1 );
    BOOST_MESSAGE( Grappa::current_worker() << " with task(local) called fetch add=" << result );
    if ( result == num_tasks-1 ) {
        BOOST_MESSAGE( Grappa::current_worker() << " with task(local) " << mynum << " result=" << result );
        Grappa::wake( parent );
    }
}

void wakedum_f( wake_arg * unused, size_t arg_size, void * payload, size_t payload_size ) {
    if ( dummy_thr != NULL ) {
        Grappa::wake( dummy_thr );
    }
}

void task_stolen( task1_arg * arg ) {
    int mynum = arg->num;
    Worker * parent = arg->parent;

    // this task should have been stolen and running on not 0
    BOOST_CHECK( 0 != Grappa::mycore() );
    
    GlobalAddress<int64_t> dum_addr = GlobalAddress<int64_t>::TwoDimensional( &num_stolen_started, 0 );
    int64_t result_d = Grappa_delegate_fetch_and_add_word( dum_addr, 1 );
    if ( result_d == (Grappa::cores()-1)*tasks_per_node - 1 ) {
        // wake_arg wwwarg = {NULL};
        //Grappa_call_on( 0, &wakedum_f, &wwwarg);
        send_heap_message( dest, [] {
            if ( dummy_thr != NULL ) {
              Grappa::wake( dummy_thr );
            }
          } );
    }

    // wake the corresponding task on Core 0
    BOOST_MESSAGE( Grappa::current_worker() << " with task(stolen) " << mynum << " about to enter multi barrier" );
    threads[mynum] = Grappa::current_worker(); // store my Worker ptr in local global array
    multiBarrier( mynum );
    
    // increment the global counter
    GlobalAddress<int64_t> nf_addr = GlobalAddress<int64_t>::TwoDimensional( &num_finished, 0 );
    int64_t result = Grappa_delegate_fetch_and_add_word( nf_addr, 1 );
    BOOST_MESSAGE( Grappa::current_worker() << " with task(stolen) called fetch add=" << result );
    if ( result == num_tasks-1 ) {
        BOOST_MESSAGE( Grappa::current_worker() << " with task(stolen) " << mynum << " result=" << result );
        //Grappa_call_on( 0, &wake_f, &wwarg );
        send_heap_message( dest, [parent] {
            Grappa::wake( parent );
          } );

    }     
}


void dummy_f( task1_arg * arg ) {
    // must wait until all stolen tasks start
    BOOST_MESSAGE( "dummy start" );
    while ( num_stolen_started < (Grappa::cores()-1)*tasks_per_node) {
        dummy_thr = Grappa::current_worker();
        Grappa::suspend();
    }
    BOOST_MESSAGE( "dummy done" );
}

void user_main(void * args ) 
{
  num_tasks = tasks_per_node * Grappa::cores();

  task1_arg argss[num_tasks];
  for (int no = 1; no < Grappa::cores(); no++) {
      for (int ta = 0; ta<tasks_per_node; ta++) {
          int index = (Grappa::cores() * tasks_per_node) + ta;
          argss[index] = { ta, me };
          Grappa_publicTask( &task_stolen, &argss[index] );
      }
  }
  for (int ta = 0; ta<tasks_per_node; ta++) {
    argss[ta] = { ta, Grappa::current_worker() };
    Grappa_publicTask( &task_local, &argss[ta] );
  }
  // another task to allow the last steal to happen ( localdepth > 2*chunkize)
  Grappa_publicTask( &dummy_f, &argss[0] );

  // wait for tasks to finish
  //Grappa_waitForTasks( );

  // tell all nodes to close communication
  //Grappa_signal_done( );

  BOOST_MESSAGE( "user main is exiting" );

}


BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  Grappa_activate();

  // force certain run parameters
  //BOOST_CHECK_EQUAL( FLAGS_chunk_size, 4 );
  //BOOST_CHECK_EQUAL( FLAGS_num_starting_workers, 4 );

  DVLOG(1) << "Spawning user main Worker....";
  Grappa_run_user_main( &user_main, (void*)NULL );
  BOOST_CHECK( Grappa_done() == true );

  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

