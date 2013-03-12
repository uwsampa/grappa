
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
        bool started = false;

        final = new CompletionEvent(FLAGS_num_test_workers);
        task_barrier = new CompletionEvent(FLAGS_num_test_workers);

        for ( uint64_t t=0; t<FLAGS_num_test_workers; t++ ) {
          privateTask( [&started,&start] {
            // wait for all to start (to hack scheduler yield)
            task_barrier->complete();
            task_barrier->wait();

            // first task to exit the local barrier will start the timer
            if ( !started ) {
              start = Grappa_walltime();
              started = true;
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

      BOOST_MESSAGE( "cores_time_avg = " << r.runtime_avg
                      << ", cores_time_max = " << r.runtime_max
                      << ", cores_time_min = " << r.runtime_min);
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

