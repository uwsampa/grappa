// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef TASK_HPP
#define TASK_HPP

#include <iostream>
#include <deque>
#include "Worker.hpp"
#include "StatisticsTools.hpp"

#ifdef VTRACE
#include <vt_user.h>
#endif
#ifdef VTRACE_SAMPLED
#include <vt_user.h>
#endif


/* metrics */
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, single_steal_successes_);
GRAPPA_DECLARE_STAT(SummarizingStatistic<uint64_t>, steal_amt_);
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, single_steal_fails_);
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, session_steal_successes_);
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, session_steal_fails_);
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, acquire_successes_);
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, acquire_fails_);
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, releases_);
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, public_tasks_dequeued_);
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, private_tasks_dequeued_);
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, remote_private_tasks_spawned_);

GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, globalq_pushes_);
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, globalq_push_attempts_);
GRAPPA_DECLARE_STAT(SummarizingStatistic<uint64_t>, globalq_elements_pushed_);
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, globalq_pulls_);
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, globalq_pull_attempts_);
GRAPPA_DECLARE_STAT(SummarizingStatistic<uint64_t>, globalq_elements_pulled_);

GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, workshare_tests_);
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, workshares_initiated_);
GRAPPA_DECLARE_STAT(SummarizingStatistic<uint64_t>, workshares_initiated_received_elements_);
GRAPPA_DECLARE_STAT(SummarizingStatistic<uint64_t>, workshares_initiated_pushed_elements_);

// number of calls to sample() 
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, sample_calls);



namespace Grappa {
  namespace impl {

// forward declaration of Grappa Node
typedef int16_t Node;

/// Represents work to be done. 
/// A function pointer and 3 64-bit arguments.
class Task {

  private:
    // function pointer that takes three arbitrary 64-bit (ptr-size) arguments
    void (* fn_p)(void*,void*,void*);

    // 3 64-bit arguments
    // This fixed number of arguments is useful for common uses
    void* arg0;
    void* arg1;
    void* arg2;

    std::ostream& dump ( std::ostream& o ) const {
      return o << "Task{"
        << " fn_p=" << fn_p
        << ", arg0=" << std::dec << arg0
        << ", arg1=" << std::dec << arg1
        << ", arg2=" << std::dec << arg2
        << "}";
    }

  public:
    /// Default constructor; only used for making space for copying
    Task () {}

    /// New task creation constructor.
    /// 
    /// @param fn_p function pointer taking three 64-bit arguments, no return value
    /// @param arg0 first task argument
    /// @param arg1 second task argument
    /// @param arg2 third task argument
    Task (void (* fn_p)(void*, void*, void*), void* arg0, void* arg1, void* arg2) 
      : fn_p ( fn_p )
        , arg0 ( arg0 )
        , arg1 ( arg1 )
        , arg2 ( arg2 ) { }

    /// Execute the task.
    /// Calls the function pointer on the provided arguments.
    void execute( ) {
      CHECK( fn_p!=NULL ) << "fn_p=" << (void*)fn_p << "\narg0=" << (void*)arg0 << "\narg1=" << (void*)arg1 << "\narg2=" << (void*)arg2;
      fn_p( arg0, arg1, arg2 );  // NOTE: this executes 1-parameter function's with 3 args
    }

    void on_stolen( ) {
      DVLOG(4) << "Stolen " << *this;
    }

    friend std::ostream& operator<<( std::ostream& o, const Task& t );
};

/// Convenience function for creating a new task.
/// This function is callable as type-safe but creates an anonymous task object.
/// 
/// @tparam A0 first argument type
/// @tparam A1 second argument type
/// @tparam A2 third argument type
/// 
/// @param arg0 first task argument
/// @param arg1 second task argument
/// @param arg2 third task argument
///
/// @return a new anonymous-type task
template < typename A0, typename A1, typename A2 >
static Task createTask( void (*fn_p)(A0, A1, A2), A0 arg0, A1 arg1, A2 arg2 ) {
  Task t( reinterpret_cast< void (*) (void*, void*, void*) >( fn_p ), (void*)arg0, (void*)arg1, (void*)arg2 );
  return t;
}

/// Keeps track of tasks, pairing workers with tasks, and load balancing.
class TaskManager {
  private:
    /// queue for tasks assigned specifically to this Node
    std::deque<Task> privateQ; 

    /// indicates that all tasks *should* be finished
    /// and termination can occur
    bool workDone;


    /// machine-local id (to support hierarchical dynamic load balancing)
    Node localId;

    bool all_terminate;

    /// stealing on/off
    bool doSteal;   

    /// steal lock
    bool stealLock;

    /// sharing on/off
    bool doShare;    

    /// work share lock
    bool wshareLock; 

    /// global queue on/off
    bool doGQ;      
    bool gqPushLock;     // global queue push lock
    bool gqPullLock;     // global queue pull lock

    /// local neighbors (to support hierarchical dynamic load balancing)
    Node* neighbors;

    /// number of Nodes on local machine (to support hierarchical dynamic load balancing)
    Node numLocalNodes;

    /// next victim to steal from (for selection by pseudo-random permutation)
    int64_t nextVictimIndex;

    /// load balancing batch size
    int chunkSize;

    /// Flags to save whether a worker thinks there
    /// could be work or if other workers should not
    /// also try.
    bool sharedMayHaveWork;

    /// Push public task
    void push_public_task( Task t );

    /// @return true if local shared queue has elements
    bool publicHasEle() const;

    /// @return true if Node-private queue has elements
    bool privateHasEle() const {
      return !privateQ.empty();
    }

    // "queue" operations
    bool tryConsumeLocal( Task * result );
    bool tryConsumeShared( Task * result );
    bool waitConsumeAny( Task * result );

    // helper operations; called each in once place
    // for sampling profiler to distinguish code by function
    void checkPull();
    void tryPushToGlobal();
    void checkWorkShare();


    /// Output internal state.
    /// 
    /// @param o existing output stream to append to
    /// 
    /// @return new output stream 
    std::ostream& dump( std::ostream& o = std::cout, const char * terminator = "" ) const;

  public:

        void record_successful_steal_session() {
          session_steal_successes_++;
        }

        void record_failed_steal_session() {
          session_steal_fails_++;
        }

        void record_successful_steal( int64_t amount ) {
          single_steal_successes_++;
          steal_amt_+= amount;
        }

        void record_failed_steal() {
          single_steal_fails_++;
        }

        void record_successful_acquire() {
          acquire_successes_++;
        }

        void record_failed_acquire() {
          acquire_fails_++;
        }

        void record_release() {
          releases_++;
        }

        void record_public_task_dequeue() {
          public_tasks_dequeued_++;
        }

        void record_private_task_dequeue() {
          private_tasks_dequeued_++;
        }

        void record_globalq_push( uint64_t amount, bool success ) {
          globalq_push_attempts_ += 1;
          if (success) {
            globalq_elements_pushed_ += amount;
            globalq_pushes_ += 1;
          }
        }

        void record_globalq_pull_start( ) {
          globalq_pull_attempts_ += 1;
        }

        void record_globalq_pull( uint64_t amount ) {
          if ( amount > 0 ) {
            globalq_elements_pulled_ += amount;
            globalq_pulls_ += 1;
          }
        }

        void record_workshare_test() {
          workshare_tests_++;
        }

        void record_remote_private_task_spawn() {
          remote_private_tasks_spawned_++;
        }

        void record_workshare( int64_t change ) {
          workshares_initiated_ += 1;
          if ( change < 0 ) {
            workshares_initiated_pushed_elements_+= (-change);
          } else {
            workshares_initiated_received_elements_ += change;
          }
        }

    //TaskManager (bool doSteal, Node localId, Node* neighbors, Node numLocalNodes, int chunkSize, int cbint);
    TaskManager();
    void init (Node localId, Node* neighbors, Node numLocalNodes);
    void activate();

    /// @return true if work is considered finished and
    ///         the task system is terminating
    bool isWorkDone() {
      return workDone;
    }

    bool global_queue_on() {
      return doGQ;
    }

    /*TODO return value?*/
    template < typename A0, typename A1, typename A2 > 
      void spawnPublic( void (*f)(A0, A1, A2), A0 arg0, A1 arg1, A2 arg2 );

    /*TODO return value?*/ 
    template < typename A0, typename A1, typename A2 > 
      void spawnLocalPrivate( void (*f)(A0, A1, A2), A0 arg0, A1 arg1, A2 arg2 );

    /*TODO return value?*/ 
    template < typename A0, typename A1, typename A2 > 
      void spawnRemotePrivate( void (*f)(A0, A1, A2), A0 arg0, A1 arg1, A2 arg2 );

    bool getWork ( Task * result );

    bool available ( ) const;
    bool local_available ( ) const;

    void finish();

    friend std::ostream& operator<<( std::ostream& o, const TaskManager& tm );

    void signal_termination( );
};

/// Whether work possibly exists locally or globally
inline bool TaskManager::available( ) const {
  VLOG(6) << " publicHasEle()=" << publicHasEle()
    << " privateHasEle()=" << privateHasEle();
  return privateHasEle() 
    || publicHasEle()
    || (doSteal && stealLock )
    || (doShare && wshareLock )
    || (doGQ    && gqPullLock );
}

/// Whether work exists locally
inline bool TaskManager::local_available( ) const {
  VLOG(6) << " publicHasEle()=" << publicHasEle()
    << " privateHasEle()=" << privateHasEle();
  return privateHasEle() 
    || publicHasEle();
}

/// Create a task in the global task pool.
/// Will start out in local partition of global task pool.
///
/// @tparam A0 type of first task argument
/// @tparam A1 type of second task argument
/// @tparam A2 type of third task argument
///
/// @param f function pointer for the new task
/// @param arg0 first task argument
/// @param arg1 second task argument
/// @param arg2 third task argument
template < typename A0, typename A1, typename A2 > 
inline void TaskManager::spawnPublic( void (*f)(A0, A1, A2), A0 arg0, A1 arg1, A2 arg2 ) {
  Task newtask = createTask(f, arg0, arg1, arg2 );
  push_public_task( newtask );
}


/// Create a task in the local private task pool.
/// Should NOT be called from the context of an AM handler.
///
/// @tparam A0 type of first task argument
/// @tparam A1 type of second task argument
/// @tparam A2 type of third task argument
///
/// @param f function pointer for the new task
/// @param arg0 first task argument
/// @param arg1 second task argument
/// @param arg2 third task argument
template < typename A0, typename A1, typename A2 >
inline void TaskManager::spawnLocalPrivate( void (*f)(A0, A1, A2), A0 arg0, A1 arg1, A2 arg2 ) {
  Task newtask = createTask( f, arg0, arg1, arg2 );
  privateQ.push_back( newtask );

  /// note from cbarrier implementation
  /* no notification necessary since
   * presence of a local spawn means
   * we are not in the cbarrier */
}

/// Create a task in the local private task pool.
/// Should ONLY be called from the context of an AM handler
///
/// @tparam A0 type of first task argument
/// @tparam A1 type of second task argument
/// @tparam A2 type of third task argument
///
/// @param f function pointer for the new task
/// @param arg0 first task argument
/// @param arg1 second task argument
/// @param arg2 third task argument
template < typename A0, typename A1, typename A2 > 
inline void TaskManager::spawnRemotePrivate( void (*f)(A0, A1, A2), A0 arg0, A1 arg1, A2 arg2 ) {
  Task newtask = createTask( f, arg0, arg1, arg2 );
  privateQ.push_front( newtask );
  /// note from cbarrier implementation
  /*
   * local cancel cbarrier
   */
}

/// system task manager
extern TaskManager global_task_manager;

} // namespace impl
} // namespace Grappa

#endif // TASK_HPP
