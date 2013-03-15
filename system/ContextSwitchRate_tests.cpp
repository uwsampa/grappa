
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "CompletionEvent.hpp"
#include "ParallelLoop.hpp"
#include "Collective.hpp"
#include "Statistics.hpp"

#include <string>

DECLARE_uint64( num_starting_workers );
DEFINE_uint64( num_test_workers, 4, "Number of workers for the tests");
DEFINE_uint64( iters_per_task, 10000, "Iterations per task" );
DEFINE_string( test_type, "yields", "options: {yields,sequential_updates, sequential_updates16" );  
DEFINE_uint64( private_array_size, 1, "Size of private array of 8-bytes for each task" );

using namespace Grappa;

CompletionEvent * final;
CompletionEvent * task_barrier;

struct SixteenBytes {
  uint64_t val1;
  uint64_t val2;
};

struct Cacheline {
  uint64_t val;
  char padding[56];
};

uint64_t * values8;
SixteenBytes * values16;

// Performance output of the test, not used as cumulative statistics
// Initial value 0 should make merge just use Core 0's
GRAPPA_DEFINE_STAT( SimpleStatistic<double>, context_switch_test_runtime_avg, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<double>, context_switch_test_runtime_max, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<double>, context_switch_test_runtime_min, 0 );


// core-shared counter for counting progress
uint64_t numst;
uint64_t waitCount; // TODO: for traces, change to SimpleStatistic
bool running;


BOOST_AUTO_TEST_SUITE( ContextSwitchRate_tests );

void user_main( void * args ) {
  srand((unsigned int)Grappa_walltime());

  // must have enough threads because they all join a barrier
  BOOST_CHECK( FLAGS_num_test_workers < FLAGS_num_starting_workers );

  if (FLAGS_test_type.compare("yields")==0) {
    BOOST_MESSAGE( "Test yields" );
    {
      struct runtimes_t {
        double runtime_avg, runtime_min, runtime_max;
      };
      runtimes_t r;
      
      on_all_cores( [&r] {
        // per core timing
        double start, end;
        running = false;

        final = new CompletionEvent(FLAGS_num_test_workers);
        task_barrier = new CompletionEvent(FLAGS_num_test_workers);

        for ( uint64_t t=0; t<FLAGS_num_test_workers; t++ ) {
          privateTask( [&start] {
            // wait for all to start (to hack scheduler yield)
            task_barrier->complete();
            task_barrier->wait();

            // first task to exit the local barrier will start the timer
            if ( !running ) {
              Grappa::Statistics::reset();
              start = Grappa_walltime();
              running = true;
            }

            // do the work
            for ( uint64_t i=0; i<FLAGS_iters_per_task; i++ ) { 
              Grappa_yield();
            }

            final->complete();
          });
        }
        
        BOOST_MESSAGE( "waiting" );
        final->wait();
        end = Grappa_walltime();
        double runtime = end-start;
        BOOST_MESSAGE( "took time " << runtime );

        Grappa::barrier();
        BOOST_MESSAGE( "all done" );

        // sort out timing 

        double r_sum = Grappa::allreduce<double, collective_add>( runtime );
        double r_min = Grappa::allreduce<double, collective_min>( runtime );
        double r_max = Grappa::allreduce<double, collective_max>( runtime );
        if ( Grappa::mycore()==0 ) {
          r.runtime_avg = r_sum / Grappa::cores();
          r.runtime_min = r_min;
          r.runtime_max = r_max;
        }
      });
      
      context_switch_test_runtime_avg = r.runtime_avg;
      context_switch_test_runtime_max = r.runtime_max;
      context_switch_test_runtime_min = r.runtime_min;
      Grappa::Statistics::merge_and_print();

//      Streams overlap
//      BOOST_MESSAGE( "cores_time_avg = " << r.runtime_avg
//                      << ", cores_time_max = " << r.runtime_max
//                      << ", cores_time_min = " << r.runtime_min);
    }
  } else if (FLAGS_test_type.compare("cvwakes")==0) {
    BOOST_MESSAGE( "Test cv wakes" );
    {
      struct runtimes_t {
        double runtime_avg, runtime_min, runtime_max;
      };
      runtimes_t r;

      on_all_cores( [&r] {
        // per core timing
        double start, end;

        ConditionVariable * cvs = new ConditionVariable[FLAGS_num_test_workers];
        bool * asleep = new bool[FLAGS_num_test_workers];
        for( int i=0; i<FLAGS_num_test_workers; i++) { asleep[i] = false; }

        final = new CompletionEvent(1);
        task_barrier = new CompletionEvent(FLAGS_num_test_workers);

        running = false;
        waitCount = 0;
        numst = 0;

        for ( uint64_t t=0; t<FLAGS_num_test_workers; t++ ) {
          privateTask( [asleep,&start,cvs] {
            // wait for all to start (to hack scheduler yield)
            task_barrier->complete();
            task_barrier->wait();

            // first task to exit the local barrier will start the timer
            if ( !running ) {
              // can safely reset statistics here because
              // no messages are sent between cores in the
              // timed portion
              Grappa::Statistics::reset();
              start = Grappa_walltime();
              running = true;
            }

            uint64_t tid = numst++;
              
            uint64_t partner = (tid + FLAGS_num_test_workers/2)%FLAGS_num_test_workers;
            uint64_t total_iters = FLAGS_iters_per_task*FLAGS_num_test_workers; 

            // do the work
            while( waitCount++ < total_iters ) {
              if ( asleep[partner] ) {   // TODO also test just wake up case
                Grappa::signal( &cvs[partner] );
              }
              asleep[tid] = true;
              Grappa::wait( &cvs[tid] );
              asleep[tid] = false;
            }

            // only first 
            if ( running ) {
              final->complete();  // signal to finish as soon as the parent task gets scheduled 
              running = false;
            }
          });
        }
        
        BOOST_MESSAGE( "waiting" );
        final->wait();
        end = Grappa_walltime();
        double runtime = end-start;
        BOOST_MESSAGE( "took time " << runtime );

        // wake all
        for (int i=0; i<FLAGS_num_test_workers; i++) { Grappa::signal(&cvs[i]); }
        BOOST_MESSAGE( "woke all" );

        Grappa::barrier();
        
        BOOST_MESSAGE( "all done" );

        // sort out timing 

        double r_sum = Grappa::allreduce<double, collective_add>( runtime );
        double r_min = Grappa::allreduce<double, collective_min>( runtime );
        double r_max = Grappa::allreduce<double, collective_max>( runtime );
        if ( Grappa::mycore()==0 ) {
          r.runtime_avg = r_sum / Grappa::cores();
          r.runtime_min = r_min;
          r.runtime_max = r_max;
        }
      });
        
      context_switch_test_runtime_avg = r.runtime_avg;
      context_switch_test_runtime_max = r.runtime_max;
      context_switch_test_runtime_min = r.runtime_min;
      Grappa::Statistics::merge_and_print();

//      Streams overlap
//      BOOST_MESSAGE( "cores_time_avg = " << r.runtime_avg
//                      << ", cores_time_max = " << r.runtime_max
//                      << ", cores_time_min = " << r.runtime_min);
    }
  
  } else if (FLAGS_test_type.compare("sequential_updates")==0) {
    BOOST_MESSAGE( "Test sequential_updates" );
    {

      final = new CompletionEvent(FLAGS_num_starting_workers);
      task_barrier = new CompletionEvent(FLAGS_num_starting_workers);
      values8 = new uint64_t[FLAGS_num_starting_workers];

      double start = Grappa_walltime();

      for ( uint64_t t=0; t<FLAGS_num_starting_workers; t++ ) {
        privateTask( [t] {
          // wait for all to start (to hack scheduler yield)
          task_barrier->complete();
          task_barrier->wait();

          // do the work
          for ( uint64_t i=0; i<FLAGS_iters_per_task; i++ ) { 
            values8[t] += 1;
            Grappa_yield();
          }
          final->complete();
        });
      }

      final->wait();
      double end = Grappa_walltime();

      double runtime = end-start;
      BOOST_MESSAGE( "time = " << runtime << ", avg_switch_time = " << runtime/(FLAGS_num_starting_workers*FLAGS_iters_per_task) );
    }
  } else if (FLAGS_test_type.compare("sequential_updates16")==0) {

    BOOST_MESSAGE( "Test sequential_updates16" );
    {
      final = new CompletionEvent(FLAGS_num_starting_workers);
      task_barrier = new CompletionEvent(FLAGS_num_starting_workers);
      values16 = new SixteenBytes[FLAGS_num_starting_workers];

      double start = Grappa_walltime();

      for ( uint64_t t=0; t<FLAGS_num_starting_workers; t++ ) {
        privateTask( [t] {
            // wait for all to start (to hack scheduler yield)
            task_barrier->complete();
            task_barrier->wait();

            // do the work
            for ( uint64_t i=0; i<FLAGS_iters_per_task; i++ ) { 
            values16[t].val1 += 1;
            values16[t].val2 += 1;
            Grappa_yield();
            }
            final->complete();
            });
      }

      final->wait();
      double end = Grappa_walltime();

      double runtime = end-start;
      BOOST_MESSAGE( "time = " << runtime << ", avg_switch_time = " << runtime/(FLAGS_num_starting_workers*FLAGS_iters_per_task) );
    }
  } else if (FLAGS_test_type.compare("private_array")==0) {

    BOOST_MESSAGE( "Test private_array" );
    {
      final = new CompletionEvent(FLAGS_num_starting_workers);
      task_barrier = new CompletionEvent(FLAGS_num_starting_workers);
      values8 = new uint64_t[FLAGS_num_starting_workers];

      double start = Grappa_walltime();

      for ( uint64_t t=0; t<FLAGS_num_starting_workers; t++ ) {
        privateTask( [t] {
            uint64_t myarray[FLAGS_private_array_size];

            // wait for all to start (to hack scheduler yield)
            task_barrier->complete();
            task_barrier->wait();

            // do the work
            for ( uint64_t i=0; i<FLAGS_iters_per_task; i++ ) { 
            for (uint64_t j=0; j<FLAGS_private_array_size; j++) {
            myarray[j] += 1;
            }
            Grappa_yield();
            }
            values8[t] = myarray[rand()%FLAGS_private_array_size];
            final->complete();
            });
      }

      final->wait();
      double end = Grappa_walltime();

      BOOST_MESSAGE( "result = " << values8[rand()%FLAGS_num_starting_workers] );

      double runtime = end-start;
      BOOST_MESSAGE( "time = " << runtime << ", avg_switch_time = " << runtime/(FLAGS_num_starting_workers*FLAGS_iters_per_task) );
    }
  } else if (FLAGS_test_type.compare("private_array_bycache")==0) {

    BOOST_MESSAGE( "Test private_array_bycache" );
    {
      final = new CompletionEvent(FLAGS_num_starting_workers);
      task_barrier = new CompletionEvent(FLAGS_num_starting_workers);
      values8 = new uint64_t[FLAGS_num_starting_workers];

      double start = Grappa_walltime();

      for ( uint64_t t=0; t<FLAGS_num_starting_workers; t++ ) {
        privateTask( [t] {
            Cacheline myarray[FLAGS_private_array_size];

            // wait for all to start (to hack scheduler yield)
            task_barrier->complete();
            task_barrier->wait();

            // do the work
            for ( uint64_t i=0; i<FLAGS_iters_per_task; i++ ) { 
            for (uint64_t j=0; j<FLAGS_private_array_size; j++) {
            myarray[j].val += 1;
            }
            Grappa_yield();
            }
            values8[t] = myarray[rand()%FLAGS_private_array_size].val;
            final->complete();
            });
      }

      final->wait();
      double end = Grappa_walltime();

      BOOST_MESSAGE( "result = " << values8[rand()%FLAGS_num_starting_workers] );

      double runtime = end-start;
      BOOST_MESSAGE( "time = " << runtime << ", avg_switch_time = " << runtime/(FLAGS_num_starting_workers*FLAGS_iters_per_task) );
    }
  } else {
    BOOST_CHECK( false ); // Unrecognized test_type
  }


  BOOST_MESSAGE( "user main is exiting" );
}



BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  Grappa_activate();

  DVLOG(1) << "Spawning user main Thread....";
  Grappa_run_user_main( &user_main, (void*)NULL );
  VLOG(5) << "run_user_main returned";
  CHECK( Grappa_done() );

  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

