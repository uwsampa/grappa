#include "Task.hpp"
#include "../SoftXMT.hpp"
#include "../PerformanceTools.hpp"
#include "TaskingScheduler.hpp"
#include "common.hpp"
#include "GlobalQueue.hpp"

DEFINE_bool( work_share, false, "Load balance with work-sharing between public task queues" );
DEFINE_bool( global_queue, false, "Load balance with a global queue" );
DEFINE_uint64( global_queue_threshold, 1024, "Threshold to trigger release of tasks to global queue" );

//#define MAXQUEUEDEPTH 500000
#define MAXQUEUEDEPTH (1L<<26)

TaskManager global_task_manager;

GRAPPA_DEFINE_EVENT_GROUP(task_manager);
//DEFINE_bool(TaskManager_events, true, "Enable tracing of events in TaskManager.");

TaskManager::TaskManager ( ) 
  : privateQ( )
  , workDone( false )
  , all_terminate( false )
  , doSteal( false )
  , doShare( false )
  , doGQ( false )
  , stealLock( true )
  , wshareLock( true )
  , gqLock( true )
  , nextVictimIndex( 0 )
  , stats( this )
{
}


void TaskManager::init (bool doSteal_arg, Node localId_arg, Node * neighbors_arg, Node numLocalNodes_arg, int chunkSize_arg, int cbint_arg) {
  CHECK( !(doSteal_arg && FLAGS_work_share) &&
         !(doSteal_arg && FLAGS_global_queue) &&
         !(FLAGS_work_share && FLAGS_global_queue) ) << "cannot use more than one load balancing mechanism";
  fast_srand(0);

  doSteal = doSteal_arg;
  doShare = FLAGS_work_share;
  doGQ = FLAGS_global_queue;

  // initialize public task queue
  publicQ.init( MAXQUEUEDEPTH );

  localId = localId_arg;
  neighbors = neighbors_arg;
  numLocalNodes = numLocalNodes_arg;
  chunkSize = chunkSize_arg;
  cbint = cbint_arg;

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
template bool global_queue_pull<Task>( ChunkInfo<Task> * result );
template bool global_queue_push<Task>( GlobalAddress<Task> chunk_base, uint64_t chunk_amount );

// StealQueue instantiations
template StealQueue<Task> StealQueue<Task>::steal_queue;
template GlobalQueue<Task> GlobalQueue<Task>::global_queue;

inline void TaskManager::tryPushToGlobal() {
  // push to global queue if local queue has grown large
  if ( doGQ && SoftXMT_global_queue_isInit() && gqLock ) {
    gqLock = false;
    uint64_t local_size = publicQ.depth();
    if ( local_size >= FLAGS_global_queue_threshold ) {
      uint64_t push_amount = MIN_INT( local_size/2, chunkSize );
      bool push_success = publicQ.push_global( push_amount );
      stats.record_globalq_push( push_amount, push_success );
    }
    gqLock = true;
  }
}

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

inline void TaskManager::checkWorkShare() {
  // initiate load balancing with prob=1/publicQ.depth
  if ( doShare && wshareLock ) {
    wshareLock = false;
    stats.record_workshare_test( );
    if ( publicQ.depth() == 0 || ((fast_rand()%(1<<16)) < ((1<<16)/publicQ.depth())) ) {
      Node target = fast_rand()%SoftXMT_nodes();
      if ( target == SoftXMT_mynode() ) target = (target+1)%SoftXMT_nodes(); // don't share with ourself
      DVLOG(5) << "before share: " << publicQ;
      int64_t numChange = publicQ.workShare( target );
      DVLOG(5) << "after share of " << numChange << " tasks: " << publicQ;
      stats.record_workshare( numChange );
    }
    wshareLock = true;
  }
}


/// "work queue" operations
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

DEFINE_bool( steal_idle_only, false, "Only steal when the core has no active tasks" );
static bool stealOk() {
  return (!FLAGS_steal_idle_only) || global_scheduler.active_task_count() < 4; // allow some for FJ private tasks
}

inline void TaskManager::checkPull() {
  if ( doSteal ) {
    if ( stealLock && stealOk() ) {

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

        if ( v == SoftXMT_mynode() ) continue; // don't steal from myself

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
  } else if ( doGQ && SoftXMT_global_queue_isInit() ) {
    if ( gqLock ) {
      gqLock = false;

      uint64_t num_received = publicQ.pull_global(); 
      stats.record_globalq_pull( num_received ); 

      gqLock = true;
    }
  }
}


/// Only returns when there is work or when
/// the system has no more work.
bool TaskManager::waitConsumeAny( Task * result ) {
    checkPull();

    if ( !local_available() ) {
        GRAPPA_PROFILE_CREATE( prof, "worker idle", "(suspended)", GRAPPA_SUSPEND_GROUP ); 
        GRAPPA_PROFILE_START( prof );
        if ( !SoftXMT_thread_idle() ) {
            SoftXMT_yield(); // allow polling thread to run
        } else {
            DVLOG(5) << CURRENT_THREAD << " un-idled";
        }
        GRAPPA_PROFILE_STOP( prof );
    }
    
    return false;
}


std::ostream& operator<<( std::ostream& o, const TaskManager& tm ) {
    return tm.dump( o );
}

std::ostream& operator<<( std::ostream& o, const Task& t ) {
    return t.dump( o );
}

void TaskManager::signal_termination( ) {
    workDone = true;
    SoftXMT_signal_done();
}

void TaskManager::finish() {
}

///
/// Stats
///

void TaskManager::dump_stats() {
    stats.dump();
}

#include "DictOut.hpp"
void TaskManager::TaskStatistics::dump() {
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

    std::cout << "TaskStatistics " << dout.toString() << std::endl;
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

void TaskManager::TaskStatistics::profiling_sample() {
#ifdef VTRACE_SAMPLED
  VT_COUNT_UNSIGNED_VAL( privateQ_size_vt_ev, tm->privateQ.size() );
  VT_COUNT_UNSIGNED_VAL( publicQ_size_vt_ev, publicQ.depth() );
  VT_COUNT_UNSIGNED_VAL( session_steal_successes_vt_ev, session_steal_successes_);
  VT_COUNT_UNSIGNED_VAL( session_steal_fails_vt_ev, session_steal_fails_);
  VT_COUNT_UNSIGNED_VAL( single_steal_successes_vt_ev, single_steal_successes_);
  VT_COUNT_UNSIGNED_VAL( total_steal_tasks_vt_ev, total_steal_tasks_);
  VT_COUNT_UNSIGNED_VAL( single_steal_fails_vt_ev, single_steal_fails_);
  VT_COUNT_UNSIGNED_VAL( acquire_successes_vt_ev, acquire_successes_);
  VT_COUNT_UNSIGNED_VAL( acquire_fails_vt_ev, acquire_fails_);
  VT_COUNT_UNSIGNED_VAL( releases_vt_ev, releases_);
  VT_COUNT_UNSIGNED_VAL( public_tasks_dequeued_vt_ev, public_tasks_dequeued_);
  VT_COUNT_UNSIGNED_VAL( private_tasks_dequeued_vt_ev, private_tasks_dequeued_);
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
