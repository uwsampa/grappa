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
#include "TaskingScheduler.hpp"
#include "Task.hpp"
#include "../PerformanceTools.hpp"
#include "common.hpp"
#include "Metrics.hpp"

// #include "GlobalQueue.hpp"

#include "StealQueue.hpp"
#include "../Grappa.hpp"

DEFINE_int32( chunk_size, 10, "Max amount of work transfered per load balance" );
DEFINE_string( load_balance, "none", "Type of dynamic load balancing {none (default), steal, share, gq}" );
DEFINE_uint64( global_queue_threshold, 1024, "Threshold to trigger release of tasks to global queue" );

size_t steal_queue_size = 1L<<19;  // previous values: 500000

/// local queue for being part of global task pool
#define publicQ StealQueue<Task>::steal_queue


/* metrics */
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, single_steal_successes_, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<uint64_t>, steal_amt_, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, single_steal_fails_, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, session_steal_successes_, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, session_steal_fails_,0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, acquire_successes_,0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, acquire_fails_,0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, releases_,0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, public_tasks_dequeued_,0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, private_tasks_dequeued_,0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, remote_private_tasks_spawned_,0);

GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, globalq_pushes_,0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, globalq_push_attempts_,0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<uint64_t>, globalq_elements_pushed_,0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, globalq_pulls_,0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, globalq_pull_attempts_,0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<uint64_t>, globalq_elements_pulled_,0);

GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, workshare_tests_,0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, workshares_initiated_,0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<uint64_t>, workshares_initiated_received_elements_,0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<uint64_t>, workshares_initiated_pushed_elements_,0);

// number of calls to sample() 
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, sample_calls,0);

// on-demand state
GRAPPA_DEFINE_METRIC(CallbackMetric<uint64_t>, public_queue_size, []() {
    return Grappa::impl::global_task_manager.numLocalPublicTasks();
    });

GRAPPA_DEFINE_METRIC(CallbackMetric<uint64_t>, private_queue_size, []() {
    return Grappa::impl::global_task_manager.numLocalPrivateTasks();
    });


GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, tasks_heap_allocated, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, tasks_created, 0);

namespace Grappa {
  namespace impl {

TaskManager global_task_manager;

//DEFINE_bool(TaskManager_events, true, "Enable tracing of events in TaskManager.");

/// Create an uninitialized TaskManager
/// init() must subsequently be called before fully initialized.
  TaskManager::TaskManager ( ) 
  : privateQ( )
  , workDone( false )
  , doSteal( false )
  , doShare( false )
  , doGQ( false )
  , stealLock( true )
  , wshareLock( true )
  , gqPushLock( true )
  , gqPullLock( true )
  , nextVictimIndex( 0 )
{
    
}

size_t TaskManager::estimate_footprint() const {
  return FLAGS_stack_size * FLAGS_num_starting_workers
    + sizeof(Task) * steal_queue_size;
}
    
size_t TaskManager::adjust_footprint(size_t target) {
  if (estimate_footprint() > target) {
    MASTER_ONLY LOG(WARNING) << "Adjusting to fit in target footprint: " << target << " bytes";
    while (estimate_footprint() > target) {
      // first try making the steal queue smaller
      if (steal_queue_size > 4) steal_queue_size /= 2;
      // otherwise we have to start removing workers
      else if (FLAGS_num_starting_workers > 4) FLAGS_num_starting_workers--;
    }
    MASTER_ONLY VLOG(2) << "\nAdjusted:"
      << "\n  estimated footprint:  " << estimate_footprint()
      << "\n  steal_queue_size:     " << steal_queue_size
      << "\n  num_starting_workers: " << FLAGS_num_starting_workers;
  }
  return estimate_footprint();
}

/// Initialize the task manager with runtime parameters.
void TaskManager::init ( Core localId_arg, Core * neighbors_arg, Core numLocalNodes_arg ) {
  if ( FLAGS_load_balance.compare(        "none" ) == 0 ) {
    doSteal = false; doShare = false; doGQ = false;
  } else if ( FLAGS_load_balance.compare( "steal" ) == 0 ) {
    doSteal = true; doShare = false; doGQ = false;
  } else if ( FLAGS_load_balance.compare( "share" ) == 0 ) {
    CHECK( false ) << "--load_balance=share currently unsupported; see tasks/StealQueue.hpp";
    doSteal = false; doShare = true; doGQ = false;
  } else if ( FLAGS_load_balance.compare( "gq" ) == 0 ) {
    CHECK( false ) << "--load_balance=gq currently unsupported; see tasks/StealQueue.hpp";
    doSteal = false; doShare = false; doGQ = true;
  } else {
    CHECK( false ) << "load_balance=" << FLAGS_load_balance << "; must be {none, steal, share, gq}";
  }

  fast_srand(0);

  localId = localId_arg;
  neighbors = neighbors_arg;
  numLocalNodes = numLocalNodes_arg;
  chunkSize = FLAGS_chunk_size;

  // initialize neighbors to steal permutation
  srandom(0);
  for (int i=numLocalNodes; i>=2; i--) {
    int ri = random() % i;
    Core temp = neighbors[ri];
    neighbors[ri] = neighbors[i-1];
    neighbors[i-1] = temp;
  }
}

void TaskManager::activate () {
  // initialization of public task queue during system activate()
  publicQ.activate( steal_queue_size );
}

// GlobalQueue instantiations
/// template GlobalQueue<Task> GlobalQueue<Task>::global_queue;

// StealQueue instantiations
/// template void global_queue_pull<Task>( ChunkInfo<Task> * result );
/// template bool global_queue_push<Task>( GlobalAddress<Task> chunk_base, uint64_t chunk_amount );
template StealQueue<Task> StealQueue<Task>::steal_queue;

uint64_t TaskManager::numLocalPublicTasks() const {
  return publicQ.depth();
}

uint64_t TaskManager::numLocalPrivateTasks() const {
  return privateQ.size();
}
    
/// @return true if local shared queue has elements
bool TaskManager::publicHasEle() const {
  return publicQ.depth() > 0;
}
    
std::ostream& TaskManager::dump( std::ostream& o, const char * terminator) const {
  return o << "\"TaskManager\": {" << std::endl
    << "  \"publicQ\": " << publicQ.depth( ) << std::endl
    << "  \"privateQ\": " << privateQ.size() << std::endl
    << "  \"work-may-be-available?\" " << available() << std::endl
    << "  \"sharedMayHaveWork\": " << sharedMayHaveWork << std::endl
    << "  \"workDone\": " << workDone << std::endl
    << "  \"stealLock\": " << stealLock << std::endl
    << "  \"wshareLock\": " << wshareLock << std::endl
    << "}" << terminator << std::endl;
}

/// Push public task
void TaskManager::push_public_task( Task t ) {
  publicQ.push( t );
}


inline void TaskManager::tryPushToGlobal() {
  // // push to global queue if local queue has grown large
  /// if ( doGQ && Grappa_global_queue_isInit() && gqPushLock ) {
  ///   gqPushLock = false;
  ///   uint64_t local_size = publicQ.depth();
  ///   DVLOG(3) << "Allowed to push gq: local size " << local_size;
  ///   if ( local_size >= FLAGS_global_queue_threshold ) {
  ///     DVLOG(3) << "Decided to push gq";
  ///     uint64_t push_amount = MIN_INT( local_size/2, chunkSize );
  ///     bool push_success = publicQ.push_global( push_amount );
  ///     TaskManagerMetrics::record_globalq_push( push_amount, push_success );
  ///   }
  ///   gqPushLock = true;
  /// }
}

/// Find an unstarted Task to execute.
/// 
/// @param result buffer for the returned Task
/// 
/// @return true if result is valid, otherwise there are no more Tasks
bool TaskManager::getWork( Task * result ) {
  GRAPPA_FUNCTION_PROFILE( GRAPPA_TASK_GROUP );

  while ( !workDone ) {

    tryPushToGlobal();

    if ( tryConsumeLocal( result ) ) {
      return true;
    }

    if ( waitConsumeAny( result ) ) {
      return true;
    }
  }

  return false;
}

DEFINE_double( ws_coeff, 1.0f, "bias on work sharing probability" );
inline void TaskManager::checkWorkShare() {
  // initiate load balancing with prob=1/publicQ.depth
  /// //uncomment if workshare reimplemented
  /// if ( doShare && wshareLock ) {
  ///   wshareLock = false;
  ///   TaskManagerMetrics::record_workshare_test( );
  ///   uint64_t local_size = publicQ.depth();
  ///   double divisor = local_size/FLAGS_ws_coeff;
  ///   if (divisor==0) divisor = 1.0;
  ///   if ( local_size == 0 || ((fast_rand()%(1<<16)) < ((1<<16)/divisor)) ) {
  ///     Core target = fast_rand()%Grappa::cores();
  ///     if ( target == Grappa::mycore() ) target = (target+1)%Grappa::cores(); // don't share with ourself
  ///     DVLOG(5) << "before share: " << publicQ;
  ///     uint64_t amount = MIN_INT( local_size/2, chunkSize );  // offer half or limit
  ///     int64_t numChange = publicQ.workShare( target, amount );
  ///     DVLOG(5) << "after share of " << numChange << " tasks: " << publicQ;
  ///     TaskManagerMetrics::record_workshare( numChange );
  ///   }
  ///   wshareLock = true;
  /// }
}

/// Dequeue local unstarted Task if any exist.
///
/// @param result buffer for the returned Task
///
/// @return true if returning valid Task, false if no local Task exists.
bool TaskManager::tryConsumeLocal( Task * result ) {
  if ( privateHasEle() ) {
    *result = privateQ.front();
    privateQ.pop_front();
    TaskManagerMetrics::record_private_task_dequeue();
    return true;
  } else {
    checkWorkShare();

    if ( publicHasEle() ) {
      DVLOG(5) << "consuming local task";
      *result = publicQ.peek();
      publicQ.pop( );
      TaskManagerMetrics::record_public_task_dequeue();
      return true;
    } else {
      return false;
    }
  }
}

inline void TaskManager::checkPull() {
  if ( doSteal ) {
    if ( stealLock ) {

      GRAPPA_PROFILE_CREATE( prof, "stealing", "(session)", GRAPPA_TASK_GROUP );
      GRAPPA_PROFILE_START( prof );

      // only one Worker is allowed to steal
      stealLock = false;

      VLOG(5) << Grappa::current_worker() << " trying to steal";
      int goodSteal = 0;
      Core victimId = -1;

      for ( int64_t tryCount=0; 
          tryCount < numLocalNodes && !goodSteal && !(publicHasEle() || privateHasEle() || workDone);
          tryCount++ ) {

        Core v = neighbors[nextVictimIndex];
        victimId = v;
        nextVictimIndex = (nextVictimIndex+1) % numLocalNodes;

        if ( v == Grappa::mycore() ) continue; // don't steal from myself

        goodSteal = publicQ.steal_locally(v, chunkSize);

        if (goodSteal) { TaskManagerMetrics::record_successful_steal( goodSteal ); }
        else { TaskManagerMetrics::record_failed_steal(); }
      }

      // if finished because succeeded in stealing
      if ( goodSteal ) {
        VLOG(5) << Grappa::current_worker() << " steal " << goodSteal
          << " from Core" << victimId;
        VLOG(5) << *this; 
        TaskManagerMetrics::record_successful_steal_session();

        // publicQ should have had some elements in it
        // at some point after successful steal
      } else {
        VLOG(5) << Grappa::current_worker() << " failed to steal";

        TaskManagerMetrics::record_failed_steal_session();

      }

      VLOG(5) << "left stealing loop with goodSteal=" << goodSteal
        << " last victim=" << victimId
        << " publicHasEle()=" << publicHasEle()
        << " privateHasEle()=" << privateHasEle();
      stealLock = true; // release steal lock

      /**TODO remote load balance**/

      GRAPPA_PROFILE_STOP( prof );
    }
  // } else if ( doGQ && Grappa_global_queue_isInit() ) {
  //   if ( gqPullLock ) { 
  //     // artificially limiting to 1 outstanding pull; for
  //     // now we do want a small limit since pulls are
  //     // blocking
  //     gqPullLock = false;
  // 
  //     TaskManagerMetrics::record_globalq_pull_start( );  // record the start separately because pull_global() may block Grappa::current_worker() indefinitely
  //     uint64_t num_received = publicQ.pull_global(); 
  //     TaskManagerMetrics::record_globalq_pull( num_received ); 
  // 
  //     gqPullLock = true;
  //   }
  }
}


/// Blocking dequeue of any Task from the global Task pool.
/// Only returns when there is work or when
/// the system has no more work.
///
/// @param result buffer for the returned Task
///
/// @return true if returning valid Task, false means no more work exists
///         in the system
bool TaskManager::waitConsumeAny( Task * result ) {
  checkPull();

  if ( !local_available() ) {
    GRAPPA_PROFILE_CREATE( prof, "worker idle", "(suspended)", GRAPPA_SUSPEND_GROUP ); 
    GRAPPA_PROFILE_START( prof );
    if ( !Grappa::thread_idle() ) { // TODO: change to directly use scheduler thread idle
      Grappa::yield(); // TODO: remove this, since thread_idle now suspends always
    } else {
      DVLOG(5) << Grappa::current_worker() << " un-idled";
    }
    GRAPPA_PROFILE_STOP( prof );
  }

  return false;
}

/// Print stream output of TaskManager
/// 
/// @param o output stream to append to
/// @param tm TaskManager to print
///
/// @return new output stream
std::ostream& operator<<( std::ostream& o, const TaskManager& tm ) {
  return tm.dump( o );
}

std::ostream& operator<<( std::ostream& o, const Task& t ) {
  return t.dump( o );
}

// defined in Grappa.cpp
extern void signal_done();

/// Tell the TaskManager that it should terminate.
/// Any tasks that are still in the queues are
/// not guarenteed to be executed after this returns.
void TaskManager::signal_termination( ) {
  workDone = true;
  signal_done();
}

/// Teardown.
/// Currently does nothing.
void TaskManager::finish() {
}




/* metrics */

void TaskManagerMetrics::record_successful_steal_session() {
  session_steal_successes_++;
}

void TaskManagerMetrics::record_failed_steal_session() {
  session_steal_fails_++;
}

void TaskManagerMetrics::record_successful_steal( int64_t amount ) {
  single_steal_successes_++;
  steal_amt_+= amount;
}

void TaskManagerMetrics::record_failed_steal() {
  single_steal_fails_++;
}

void TaskManagerMetrics::record_successful_acquire() {
  acquire_successes_++;
}

void TaskManagerMetrics::record_failed_acquire() {
  acquire_fails_++;
}

void TaskManagerMetrics::record_release() {
  releases_++;
}

void TaskManagerMetrics::record_public_task_dequeue() {
  public_tasks_dequeued_++;
}

void TaskManagerMetrics::record_private_task_dequeue() {
  private_tasks_dequeued_++;
}

void TaskManagerMetrics::record_globalq_push( uint64_t amount, bool success ) {
  globalq_push_attempts_ += 1;
  if (success) {
    globalq_elements_pushed_ += amount;
    globalq_pushes_ += 1;
  }
}

void TaskManagerMetrics::record_globalq_pull_start( ) {
  globalq_pull_attempts_ += 1;
}

void TaskManagerMetrics::record_globalq_pull( uint64_t amount ) {
  if ( amount > 0 ) {
    globalq_elements_pulled_ += amount;
    globalq_pulls_ += 1;
  }
}

void TaskManagerMetrics::record_workshare_test() {
  workshare_tests_++;
}

void TaskManagerMetrics::record_remote_private_task_spawn() {
  remote_private_tasks_spawned_++;
}

void TaskManagerMetrics::record_workshare( int64_t change ) {
  workshares_initiated_ += 1;
  if ( change < 0 ) {
    workshares_initiated_pushed_elements_+= (-change);
  } else {
    workshares_initiated_received_elements_ += change;
  }
}


} // impl
} // Grappa
