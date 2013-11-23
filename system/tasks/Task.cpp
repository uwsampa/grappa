// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "Task.hpp"
#include "../Grappa.hpp"
#include "../PerformanceTools.hpp"
#include "TaskingScheduler.hpp"
#include "common.hpp"
#include "Statistics.hpp"

// #include "GlobalQueue.hpp"

#include "StealQueue.hpp"

DEFINE_int32( chunk_size, 10, "Max amount of work transfered per load balance" );
DEFINE_string( load_balance, "steal", "Type of dynamic load balancing {none, steal (default), share, gq}" );
DEFINE_uint64( global_queue_threshold, 1024, "Threshold to trigger release of tasks to global queue" );

#define MAXQUEUEDEPTH (1L<<19)   // previous values: 500000

/// local queue for being part of global task pool
#define publicQ StealQueue<Task>::steal_queue


/* metrics */
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, single_steal_successes_, 0);
GRAPPA_DEFINE_STAT(SummarizingStatistic<uint64_t>, steal_amt_, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, single_steal_fails_, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, session_steal_successes_, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, session_steal_fails_,0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, acquire_successes_,0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, acquire_fails_,0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, releases_,0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, public_tasks_dequeued_,0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, private_tasks_dequeued_,0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, remote_private_tasks_spawned_,0);

GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, globalq_pushes_,0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, globalq_push_attempts_,0);
GRAPPA_DEFINE_STAT(SummarizingStatistic<uint64_t>, globalq_elements_pushed_,0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, globalq_pulls_,0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, globalq_pull_attempts_,0);
GRAPPA_DEFINE_STAT(SummarizingStatistic<uint64_t>, globalq_elements_pulled_,0);

GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, workshare_tests_,0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, workshares_initiated_,0);
GRAPPA_DEFINE_STAT(SummarizingStatistic<uint64_t>, workshares_initiated_received_elements_,0);
GRAPPA_DEFINE_STAT(SummarizingStatistic<uint64_t>, workshares_initiated_pushed_elements_,0);

// number of calls to sample() 
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, sample_calls,0);

// on-demand state
GRAPPA_DEFINE_STAT(CallbackStatistic<uint64_t>, public_queue_size, []() {
    return Grappa::impl::global_task_manager.numLocalPublicTasks();
    });

GRAPPA_DEFINE_STAT(CallbackStatistic<uint64_t>, private_queue_size, []() {
    return Grappa::impl::global_task_manager.numLocalPrivateTasks();
    });


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
  publicQ.activate( MAXQUEUEDEPTH );
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
  ///     TaskManagerStatistics::record_globalq_push( push_amount, push_success );
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
  ///   TaskManagerStatistics::record_workshare_test( );
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
  ///     TaskManagerStatistics::record_workshare( numChange );
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
    TaskManagerStatistics::record_private_task_dequeue();
    return true;
  } else {
    checkWorkShare();

    if ( publicHasEle() ) {
      DVLOG(5) << "consuming local task";
      *result = publicQ.peek();
      publicQ.pop( );
      TaskManagerStatistics::record_public_task_dequeue();
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

      // only one Thread is allowed to steal
      stealLock = false;

      VLOG(5) << CURRENT_THREAD << " trying to steal";
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

        if (goodSteal) { TaskManagerStatistics::record_successful_steal( goodSteal ); }
        else { TaskManagerStatistics::record_failed_steal(); }
      }

      // if finished because succeeded in stealing
      if ( goodSteal ) {
        VLOG(5) << CURRENT_THREAD << " steal " << goodSteal
          << " from Core" << victimId;
        VLOG(5) << *this; 
        TaskManagerStatistics::record_successful_steal_session();

        // publicQ should have had some elements in it
        // at some point after successful steal
      } else {
        VLOG(5) << CURRENT_THREAD << " failed to steal";

        TaskManagerStatistics::record_failed_steal_session();

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
  //     TaskManagerStatistics::record_globalq_pull_start( );  // record the start separately because pull_global() may block CURRENT_THREAD indefinitely
  //     uint64_t num_received = publicQ.pull_global(); 
  //     TaskManagerStatistics::record_globalq_pull( num_received ); 
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
    if ( !Grappa_thread_idle() ) { // TODO: change to directly use scheduler thread idle
      Grappa_yield(); // TODO: remove this, since thread_idle now suspends always
    } else {
      DVLOG(5) << CURRENT_THREAD << " un-idled";
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

/// Tell the TaskManager that it should terminate.
/// Any tasks that are still in the queues are
/// not guarenteed to be executed after this returns.
void TaskManager::signal_termination( ) {
  workDone = true;
  Grappa_signal_done();
}

/// Teardown.
/// Currently does nothing.
void TaskManager::finish() {
}




/* metrics */

void TaskManagerStatistics::record_successful_steal_session() {
  session_steal_successes_++;
}

void TaskManagerStatistics::record_failed_steal_session() {
  session_steal_fails_++;
}

void TaskManagerStatistics::record_successful_steal( int64_t amount ) {
  single_steal_successes_++;
  steal_amt_+= amount;
}

void TaskManagerStatistics::record_failed_steal() {
  single_steal_fails_++;
}

void TaskManagerStatistics::record_successful_acquire() {
  acquire_successes_++;
}

void TaskManagerStatistics::record_failed_acquire() {
  acquire_fails_++;
}

void TaskManagerStatistics::record_release() {
  releases_++;
}

void TaskManagerStatistics::record_public_task_dequeue() {
  public_tasks_dequeued_++;
}

void TaskManagerStatistics::record_private_task_dequeue() {
  private_tasks_dequeued_++;
}

void TaskManagerStatistics::record_globalq_push( uint64_t amount, bool success ) {
  globalq_push_attempts_ += 1;
  if (success) {
    globalq_elements_pushed_ += amount;
    globalq_pushes_ += 1;
  }
}

void TaskManagerStatistics::record_globalq_pull_start( ) {
  globalq_pull_attempts_ += 1;
}

void TaskManagerStatistics::record_globalq_pull( uint64_t amount ) {
  if ( amount > 0 ) {
    globalq_elements_pulled_ += amount;
    globalq_pulls_ += 1;
  }
}

void TaskManagerStatistics::record_workshare_test() {
  workshare_tests_++;
}

void TaskManagerStatistics::record_remote_private_task_spawn() {
  remote_private_tasks_spawned_++;
}

void TaskManagerStatistics::record_workshare( int64_t change ) {
  workshares_initiated_ += 1;
  if ( change < 0 ) {
    workshares_initiated_pushed_elements_+= (-change);
  } else {
    workshares_initiated_received_elements_ += change;
  }
}


} // impl
} // Grappa
