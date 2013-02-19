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
#include "GlobalQueue.hpp"

DEFINE_int32( chunk_size, 10, "Max amount of work transfered per load balance" );
DEFINE_string( load_balance, "steal", "Type of dynamic load balancing {none, steal (default), share, gq}" );
DEFINE_uint64( global_queue_threshold, 1024, "Threshold to trigger release of tasks to global queue" );

#define MAXQUEUEDEPTH (1L<<26)   // previous values: 500000

namespace Grappa {
  namespace impl {

TaskManager global_task_manager;

GRAPPA_DEFINE_EVENT_GROUP(task_manager);
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
    , stats( this )
{
}


/// Initialize the task manager with runtime parameters.
void TaskManager::init ( Node localId_arg, Node * neighbors_arg, Node numLocalNodes_arg ) {
  if ( FLAGS_load_balance.compare(        "none" ) == 0 ) {
    doSteal = false; doShare = false; doGQ = false;
  } else if ( FLAGS_load_balance.compare( "steal" ) == 0 ) {
    doSteal = true; doShare = false; doGQ = false;
  } else if ( FLAGS_load_balance.compare( "share" ) == 0 ) {
    doSteal = false; doShare = true; doGQ = false;
  } else if ( FLAGS_load_balance.compare( "gq" ) == 0 ) {
    doSteal = false; doShare = false; doGQ = true;
  } else {
    CHECK( false ) << "load_balance=" << FLAGS_load_balance << "; must be {none, steal, share, gq}";
  }

  fast_srand(0);

  // initialize public task queue
  publicQ.init( MAXQUEUEDEPTH );

  localId = localId_arg;
  neighbors = neighbors_arg;
  numLocalNodes = numLocalNodes_arg;
  chunkSize = FLAGS_chunk_size;

  // initialize neighbors to steal permutation
  srandom(0);
  for (int i=numLocalNodes; i>=2; i--) {
    int ri = random() % i;
    Node temp = neighbors[ri];
    neighbors[ri] = neighbors[i-1];
    neighbors[i-1] = temp;
  }
}

// GlobalQueue instantiations
template void global_queue_pull<Task>( ChunkInfo<Task> * result );
template bool global_queue_push<Task>( GlobalAddress<Task> chunk_base, uint64_t chunk_amount );

// StealQueue instantiations
template StealQueue<Task> StealQueue<Task>::steal_queue;
template GlobalQueue<Task> GlobalQueue<Task>::global_queue;

inline void TaskManager::tryPushToGlobal() {
  // push to global queue if local queue has grown large
  if ( doGQ && Grappa_global_queue_isInit() && gqPushLock ) {
    gqPushLock = false;
    uint64_t local_size = publicQ.depth();
    DVLOG(3) << "Allowed to push gq: local size " << local_size;
    if ( local_size >= FLAGS_global_queue_threshold ) {
      DVLOG(3) << "Decided to push gq";
      uint64_t push_amount = MIN_INT( local_size/2, chunkSize );
      bool push_success = publicQ.push_global( push_amount );
      stats.record_globalq_push( push_amount, push_success );
    }
    gqPushLock = true;
  }
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
  if ( doShare && wshareLock ) {
    wshareLock = false;
    stats.record_workshare_test( );
    uint64_t local_size = publicQ.depth();
    double divisor = local_size/FLAGS_ws_coeff;
    if (divisor==0) divisor = 1.0;
    if ( local_size == 0 || ((fast_rand()%(1<<16)) < ((1<<16)/divisor)) ) {
      Node target = fast_rand()%Grappa_nodes();
      if ( target == Grappa_mynode() ) target = (target+1)%Grappa_nodes(); // don't share with ourself
      DVLOG(5) << "before share: " << publicQ;
      uint64_t amount = MIN_INT( local_size/2, chunkSize );  // offer half or limit
      int64_t numChange = publicQ.workShare( target, amount );
      DVLOG(5) << "after share of " << numChange << " tasks: " << publicQ;
      stats.record_workshare( numChange );
    }
    wshareLock = true;
  }
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
    stats.record_private_task_dequeue();
    return true;
  } else {
    checkWorkShare();

    if ( publicHasEle() ) {
      DVLOG(5) << "consuming local task";
      *result = publicQ.peek();
      publicQ.pop( );
      stats.record_public_task_dequeue();
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
      Node victimId = -1;

      for ( int64_t tryCount=0; 
          tryCount < numLocalNodes && !goodSteal && !(publicHasEle() || privateHasEle() || workDone);
          tryCount++ ) {

        Node v = neighbors[nextVictimIndex];
        victimId = v;
        nextVictimIndex = (nextVictimIndex+1) % numLocalNodes;

        if ( v == Grappa_mynode() ) continue; // don't steal from myself

        goodSteal = publicQ.steal_locally(v, chunkSize);

        if (goodSteal) { stats.record_successful_steal( goodSteal ); }
        else { stats.record_failed_steal(); }
      }

      // if finished because succeeded in stealing
      if ( goodSteal ) {
        VLOG(5) << CURRENT_THREAD << " steal " << goodSteal
          << " from Node" << victimId;
        VLOG(5) << *this; 
        stats.record_successful_steal_session();

        // publicQ should have had some elements in it
        // at some point after successful steal
      } else {
        VLOG(5) << CURRENT_THREAD << " failed to steal";

        stats.record_failed_steal_session();

      }

      VLOG(5) << "left stealing loop with goodSteal=" << goodSteal
        << " last victim=" << victimId
        << " publicHasEle()=" << publicHasEle()
        << " privateHasEle()=" << privateHasEle();
      stealLock = true; // release steal lock

      /**TODO remote load balance**/

      GRAPPA_PROFILE_STOP( prof );
    }
  } else if ( doGQ && Grappa_global_queue_isInit() ) {
    if ( gqPullLock ) { 
      // artificially limiting to 1 outstanding pull; for
      // now we do want a small limit since pulls are
      // blocking
      gqPullLock = false;

      stats.record_globalq_pull_start( );  // record the start separately because pull_global() may block CURRENT_THREAD indefinitely
      uint64_t num_received = publicQ.pull_global(); 
      stats.record_globalq_pull( num_received ); 

      gqPullLock = true;
    }
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

///
/// Stats
///

/// Print statistics.
void TaskManager::dump_stats( std::ostream& o = std::cout, const char * terminator = "" ) {
  stats.dump( o, terminator );
}

/// Print statistics in dictionary format.
/// { name1:value1, name2:value2, ... }
#include "DictOut.hpp"
void TaskManager::TaskStatistics::dump( std::ostream& o = std::cout, const char * terminator = "" ) {
  DictOut dout;
  DICT_ADD(dout, session_steal_successes_);
  DICT_ADD(dout, session_steal_fails_);
  DICT_ADD(dout, single_steal_successes_);
  DICT_ADD_STAT_TOTAL(dout, steal_amt_);
  DICT_ADD(dout, single_steal_fails_);
  DICT_ADD(dout, acquire_successes_);
  DICT_ADD(dout, acquire_fails_);
  DICT_ADD(dout, releases_);
  DICT_ADD(dout, public_tasks_dequeued_);
  DICT_ADD(dout, private_tasks_dequeued_);
  DICT_ADD(dout, remote_private_tasks_spawned_);

  DICT_ADD( dout, globalq_pushes_ );
  DICT_ADD( dout, globalq_push_attempts_ );
  DICT_ADD_STAT_TOTAL( dout, globalq_elements_pushed_ );
  DICT_ADD( dout, globalq_pulls_ );
  DICT_ADD( dout, globalq_pull_attempts_ );
  DICT_ADD_STAT_TOTAL( dout, globalq_elements_pulled_ );

  DICT_ADD( dout, workshare_tests_ );
  DICT_ADD( dout, workshares_initiated_ );
  DICT_ADD_STAT_TOTAL( dout, workshares_initiated_received_elements_ );
  DICT_ADD_STAT_TOTAL( dout, workshares_initiated_pushed_elements_ );

  o << "   \"TaskStatistics\": " << dout.toString() << terminator << std::endl;
}

void TaskManager::TaskStatistics::sample() {
  GRAPPA_EVENT(privateQ_size_ev,       "privateQ size sample",  SAMPLE_RATE, task_manager, tm->privateQ.size());
  GRAPPA_EVENT(publicQ_size_ev,  "publicQ size sample",  SAMPLE_RATE, task_manager, publicQ.depth());

#ifdef VTRACE
  //VT_COUNT_UNSIGNED_VAL( privateQ_size_vt_ev, tm->privateQ.size() );
  //VT_COUNT_UNSIGNED_VAL( publicQ_size_vt_ev, tm->publicQ.depth() );
#endif
  //    sample_calls++;
  //    /* todo: avgs */
  //
  //#ifdef GRAPPA_TRACE
  //    if ((sample_calls % 1) == 0) {
  //      TAU_REGISTER_EVENT(privateQ_size_ev, "privateQ size sample");
  //      TAU_REGISTER_EVENT(publicQ_size_ev, "publicQ size sample");
  //      
  //      TAU_EVENT(privateQ_size_ev, tm->privateQ.size());
  //      TAU_EVENT(publicQ_size_ev, tm->publicQ size);
  //    }
  //#endif
}

/// Take a sample of statistics and other state.
void TaskManager::TaskStatistics::profiling_sample() {
#ifdef VTRACE_SAMPLED
  VT_COUNT_UNSIGNED_VAL( privateQ_size_vt_ev, tm->privateQ.size() );
  VT_COUNT_UNSIGNED_VAL( publicQ_size_vt_ev, publicQ.depth() );
  VT_COUNT_UNSIGNED_VAL( session_steal_successes_vt_ev, session_steal_successes_);
  VT_COUNT_UNSIGNED_VAL( session_steal_fails_vt_ev, session_steal_fails_);
  VT_COUNT_UNSIGNED_VAL( single_steal_successes_vt_ev, single_steal_successes_);
  VT_COUNT_UNSIGNED_VAL( total_steal_tasks_vt_ev, steal_amt_.getTotal());
  VT_COUNT_UNSIGNED_VAL( single_steal_fails_vt_ev, single_steal_fails_);
  VT_COUNT_UNSIGNED_VAL( acquire_successes_vt_ev, acquire_successes_);
  VT_COUNT_UNSIGNED_VAL( acquire_fails_vt_ev, acquire_fails_);
  VT_COUNT_UNSIGNED_VAL( releases_vt_ev, releases_);
  VT_COUNT_UNSIGNED_VAL( public_tasks_dequeued_vt_ev, public_tasks_dequeued_);
  VT_COUNT_UNSIGNED_VAL( private_tasks_dequeued_vt_ev, private_tasks_dequeued_);
  VT_COUNT_UNSIGNED_VAL( remote_private_tasks_spawned_vt_ev, remote_private_tasks_spawned_);

  VT_COUNT_UNSIGNED_VAL( globalq_pushes_vt_ev, globalq_pushes_ );
  VT_COUNT_UNSIGNED_VAL( globalq_push_attempts_vt_ev, globalq_push_attempts_ );
  VT_COUNT_UNSIGNED_VAL( globalq_elements_pushed_vt_ev, globalq_elements_pushed_.getTotal() );
  VT_COUNT_UNSIGNED_VAL( globalq_pulls_vt_ev, globalq_pulls_ );
  VT_COUNT_UNSIGNED_VAL( globalq_pull_attempts_vt_ev, globalq_pull_attempts_ );
  VT_COUNT_UNSIGNED_VAL( globalq_elements_pulled_vt_ev, globalq_elements_pulled_.getTotal() );

  VT_COUNT_UNSIGNED_VAL( shares_initiated_vt_ev, workshares_initiated_ );
  VT_COUNT_UNSIGNED_VAL( shares_received_elements_vt_ev, workshares_initiated_received_elements_.getTotal() );
  VT_COUNT_UNSIGNED_VAL( shares_pushed_elements_vt_ev, workshares_initiated_pushed_elements_.getTotal() );
#endif
}

void TaskManager::TaskStatistics::merge(const TaskManager::TaskStatistics * other) {
  session_steal_successes_ += other->session_steal_successes_;
  session_steal_fails_ += other->session_steal_fails_;
  single_steal_successes_ += other->single_steal_successes_;
  MERGE_STAT_TOTAL( steal_amt_, other );
  single_steal_fails_ += other->single_steal_fails_;
  acquire_successes_ += other->acquire_successes_;
  acquire_fails_ += other->acquire_fails_;
  releases_ += other->releases_;
  public_tasks_dequeued_ += other->public_tasks_dequeued_;
  private_tasks_dequeued_ += other->private_tasks_dequeued_;
  remote_private_tasks_spawned_ += other->remote_private_tasks_spawned_;

  globalq_pushes_                        += other->globalq_pushes_;
  globalq_push_attempts_                 += other->globalq_push_attempts_;
  MERGE_STAT_TOTAL( globalq_elements_pushed_, other );
  globalq_pulls_                        += other->globalq_pulls_;
  globalq_pull_attempts_                 += other->globalq_pull_attempts_;
  MERGE_STAT_TOTAL( globalq_elements_pulled_, other );

  workshare_tests_ += other->workshare_tests_;
  workshares_initiated_ += other->workshares_initiated_;
  MERGE_STAT_TOTAL( workshares_initiated_received_elements_, other );

  MERGE_STAT_TOTAL( workshares_initiated_pushed_elements_, other );  
}


void TaskManager::reset_stats() {
  stats.reset();
}

void TaskManager::TaskStatistics::reset() {
  single_steal_successes_ =0;
  steal_amt_.reset();
  single_steal_fails_ =0;
  session_steal_successes_ =0;
  session_steal_fails_ =0;
  acquire_successes_ =0;
  acquire_fails_ =0;
  releases_ =0;
  public_tasks_dequeued_ =0;
  private_tasks_dequeued_ =0;
  remote_private_tasks_spawned_ =0;

  globalq_pushes_ = 0;
  globalq_push_attempts_ = 0;
  globalq_elements_pushed_.reset();
  globalq_pulls_ = 0;
  globalq_pull_attempts_ = 0;
  globalq_elements_pulled_.reset();

  workshare_tests_ = 0;
  workshares_initiated_ = 0;
  workshares_initiated_received_elements_.reset();
  workshares_initiated_pushed_elements_.reset();

  sample_calls =0;
}

} // impl
} // Grappa
