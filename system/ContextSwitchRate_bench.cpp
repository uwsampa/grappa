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


#include "Grappa.hpp"
#include "CompletionEvent.hpp"
#include "ParallelLoop.hpp"
#include "Collective.hpp"
#include "Metrics.hpp"

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
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, context_switch_test_runtime_avg, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, context_switch_test_runtime_max, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, context_switch_test_runtime_min, 0 );


// core-shared counter for counting progress
uint64_t numst;
uint64_t waitCount; // TODO: for traces, change to SimpleMetric
bool running;


int main(int argc, char* argv[]) {
  Grappa::init(&argc, &argv);
  Grappa::run([]{
    srand((unsigned int)Grappa::walltime());

    // must have enough threads because they all join a barrier
    CHECK( FLAGS_num_test_workers < FLAGS_num_starting_workers );

    if (FLAGS_test_type.compare("yields")==0) {
      LOG(INFO) << ( "Test yields" );
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
            spawn( [&start] {
              // wait for all to start (to hack scheduler yield)
            //  task_barrier->complete();
             // task_barrier->wait();

              // first task to exit the local barrier will start the timer
              if ( !running ) {
                Grappa::Metrics::reset();
                start = Grappa::walltime();
                running = true;
              }

              // do the work
              for ( uint64_t i=0; i<FLAGS_iters_per_task; i++ ) { 
                Grappa::yield();
              }

              final->complete();
            });
          }
        
          LOG(INFO) << ( "waiting" );
          final->wait();
          end = Grappa::walltime();
          double runtime = end-start;
          LOG(INFO) << "took time " << runtime;

          Grappa::barrier();
          LOG(INFO) << ( "all done" );

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
        Grappa::Metrics::merge_and_print();

  //      Streams overlap
  //      BOOST_MESSAGE( "cores_time_avg = " << r.runtime_avg
  //                      << ", cores_time_max = " << r.runtime_max
  //                      << ", cores_time_min = " << r.runtime_min);
      }
    } else if (FLAGS_test_type.compare("cvwakes")==0) {
      LOG(INFO) << ( "Test cv wakes" );
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
            spawn( [asleep,&start,cvs] {
              // wait for all to start (to hack scheduler yield)
              //task_barrier->complete();
              //task_barrier->wait();

              // first task to exit the local barrier will start the timer
              if ( !running ) {
                // can safely reset statistics here because
                // no messages are sent between cores in the
                // timed portion
                Grappa::Metrics::reset();
                start = Grappa::walltime();
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
        
          LOG(INFO) << ( "waiting" );
          final->wait();
          end = Grappa::walltime();
          double runtime = end-start;
          LOG(INFO) << "took time " << runtime;

          // wake all
          for (int i=0; i<FLAGS_num_test_workers; i++) { Grappa::signal(&cvs[i]); }
          LOG(INFO) << ( "woke all" );

          Grappa::barrier();
        
          LOG(INFO) << ( "all done" );

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
        Grappa::Metrics::merge_and_print();

  //      Streams overlap
  //      BOOST_MESSAGE( "cores_time_avg = " << r.runtime_avg
  //                      << ", cores_time_max = " << r.runtime_max
  //                      << ", cores_time_min = " << r.runtime_min);
      }
  
    } else if (FLAGS_test_type.compare("sequential_updates")==0) {
      LOG(INFO) << ( "Test sequential_updates" );
      {

        final = new CompletionEvent(FLAGS_num_starting_workers);
        task_barrier = new CompletionEvent(FLAGS_num_starting_workers);
        values8 = new uint64_t[FLAGS_num_starting_workers];

        double start = Grappa::walltime();

        for ( uint64_t t=0; t<FLAGS_num_starting_workers; t++ ) {
          spawn( [t] {
            // wait for all to start (to hack scheduler yield)
            task_barrier->complete();
            task_barrier->wait();

            // do the work
            for ( uint64_t i=0; i<FLAGS_iters_per_task; i++ ) { 
              values8[t] += 1;
              Grappa::yield();
            }
            final->complete();
          });
        }

        final->wait();
        double end = Grappa::walltime();

        double runtime = end-start;
        LOG(INFO) << "time = " << runtime << ", avg_switch_time = " << runtime/(FLAGS_num_starting_workers*FLAGS_iters_per_task);
      }
    } else if (FLAGS_test_type.compare("sequential_updates16")==0) {

      LOG(INFO) << ( "Test sequential_updates16" );
      {
        final = new CompletionEvent(FLAGS_num_starting_workers);
        task_barrier = new CompletionEvent(FLAGS_num_starting_workers);
        values16 = new SixteenBytes[FLAGS_num_starting_workers];

        double start = Grappa::walltime();

        for ( uint64_t t=0; t<FLAGS_num_starting_workers; t++ ) {
          spawn( [t] {
              // wait for all to start (to hack scheduler yield)
              task_barrier->complete();
              task_barrier->wait();

              // do the work
              for ( uint64_t i=0; i<FLAGS_iters_per_task; i++ ) { 
              values16[t].val1 += 1;
              values16[t].val2 += 1;
              Grappa::yield();
              }
              final->complete();
              });
        }

        final->wait();
        double end = Grappa::walltime();

        double runtime = end-start;
        LOG(INFO) << "time = " << runtime << ", avg_switch_time = " << runtime/(FLAGS_num_starting_workers*FLAGS_iters_per_task);
      }
    } else if (FLAGS_test_type.compare("private_array")==0) {

      LOG(INFO) << ( "Test private_array" );
      {
        final = new CompletionEvent(FLAGS_num_starting_workers);
        task_barrier = new CompletionEvent(FLAGS_num_starting_workers);
        values8 = new uint64_t[FLAGS_num_starting_workers];

        double start = Grappa::walltime();

        for ( uint64_t t=0; t<FLAGS_num_starting_workers; t++ ) {
          spawn( [t] {
              uint64_t myarray[FLAGS_private_array_size];

              // wait for all to start (to hack scheduler yield)
              task_barrier->complete();
              task_barrier->wait();

              // do the work
              for ( uint64_t i=0; i<FLAGS_iters_per_task; i++ ) { 
              for (uint64_t j=0; j<FLAGS_private_array_size; j++) {
              myarray[j] += 1;
              }
              Grappa::yield();
              }
              values8[t] = myarray[rand()%FLAGS_private_array_size];
              final->complete();
              });
        }

        final->wait();
        double end = Grappa::walltime();

        LOG(INFO) << "result = " << values8[rand()%FLAGS_num_starting_workers];

        double runtime = end-start;
        LOG(INFO) << "time = " << runtime << ", avg_switch_time = " << runtime/(FLAGS_num_starting_workers*FLAGS_iters_per_task);
      }
    } else if (FLAGS_test_type.compare("private_array_bycache")==0) {

      LOG(INFO) << ( "Test private_array_bycache" );
      {
        final = new CompletionEvent(FLAGS_num_starting_workers);
        task_barrier = new CompletionEvent(FLAGS_num_starting_workers);
        values8 = new uint64_t[FLAGS_num_starting_workers];

        double start = Grappa::walltime();

        for ( uint64_t t=0; t<FLAGS_num_starting_workers; t++ ) {
          spawn( [t] {
              Cacheline myarray[FLAGS_private_array_size];

              // wait for all to start (to hack scheduler yield)
              task_barrier->complete();
              task_barrier->wait();

              // do the work
              for ( uint64_t i=0; i<FLAGS_iters_per_task; i++ ) { 
              for (uint64_t j=0; j<FLAGS_private_array_size; j++) {
              myarray[j].val += 1;
              }
              Grappa::yield();
              }
              values8[t] = myarray[rand()%FLAGS_private_array_size].val;
              final->complete();
              });
        }

        final->wait();
        double end = Grappa::walltime();

        LOG(INFO) << "result = " << values8[rand()%FLAGS_num_starting_workers];

        double runtime = end-start;
        LOG(INFO) << "time = " << runtime << ", avg_switch_time = " << runtime/(FLAGS_num_starting_workers*FLAGS_iters_per_task);
      }
    } else {
      CHECK( false ); // Unrecognized test_type
    }


    DVLOG(5) << ( "user main is exiting" );
  });
  Grappa::finalize();
}
