// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef TASK_HPP
#define TASK_HPP

#include <iostream>
#include <deque>
#include "StealQueue.hpp"
#include "Thread.hpp"
#include "StatisticsTools.hpp"

#ifdef VTRACE
#include <vt_user.h>
#endif
#ifdef VTRACE_SAMPLED
#include <vt_user.h>
#endif

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

    /// local queue for being part of global task pool
    StealQueue<Task> publicQ;

    /// indicates that all tasks *should* be finished
    /// and termination can occur
    bool workDone;
        
    /// stealing on/off
    bool doSteal;   

    /// steal lock
    bool stealLock;

    /// machine-local id (to support hierarchical dynamic load balancing)
    Node localId;

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

    /// @return true if local shared queue has elements
    bool publicHasEle() const {
      return publicQ.depth() > 0;
    }

    /// @return true if Node-private queue has elements
    bool privateHasEle() const {
      return !privateQ.empty();
    }

    // "queue" operations
    bool tryConsumeLocal( Task * result );
    bool tryConsumeShared( Task * result );
    bool waitConsumeAny( Task * result );
        

    /// Output internal state.
    /// 
    /// @param o existing output stream to append to
    /// 
    /// @return new output stream 
    std::ostream& dump( std::ostream& o = std::cout, const char * terminator = "" ) const {
      return o << "\"TaskManager\": {" << std::endl
               << "  \"publicQ\": " << publicQ.depth( ) << std::endl
               << "  \"privateQ\": " << privateQ.size() << std::endl
               << "  \"work-may-be-available?\" " << available() << std::endl
               << "  \"sharedMayHaveWork\": " << sharedMayHaveWork << std::endl
               << "  \"workDone\": " << workDone << std::endl
               << "  \"stealLock\": " << stealLock << std::endl
               << "}";
    }

  public:
    /// Collects statistics on task execution and load balancing.
    class TaskStatistics {
      private:
        uint64_t single_steal_successes_;
        uint64_t total_steal_tasks_;
        uint64_t max_steal_amt_;
        RunningStandardDeviation stddev_steal_amt_;
        uint64_t single_steal_fails_;
        uint64_t session_steal_successes_;
        uint64_t session_steal_fails_;
        uint64_t acquire_successes_;
        uint64_t acquire_fails_;
        uint64_t releases_;
        uint64_t public_tasks_dequeued_;
        uint64_t private_tasks_dequeued_;
        uint64_t remote_private_tasks_spawned_;

        /// number of calls to sample() 
        uint64_t sample_calls;

#ifdef VTRACE_SAMPLED
        unsigned task_manager_vt_grp;
        unsigned privateQ_size_vt_ev;
        unsigned publicQ_size_vt_ev;
        unsigned session_steal_successes_vt_ev;
        unsigned session_steal_fails_vt_ev;
        unsigned single_steal_successes_vt_ev;
        unsigned total_steal_tasks_vt_ev;
        unsigned single_steal_fails_vt_ev;
        unsigned acquire_successes_vt_ev;
        unsigned acquire_fails_vt_ev;
        unsigned releases_vt_ev;
        unsigned public_tasks_dequeued_vt_ev;
        unsigned private_tasks_dequeued_vt_ev;
        unsigned remote_private_tasks_spawned_vt_ev;
#endif

        TaskManager * tm;

      public:
        /// Construct new statistics that are reset to zero.
        TaskStatistics(TaskManager * task_manager)
          : single_steal_successes_ (0)
            , total_steal_tasks_ (0)
            , max_steal_amt_ (0)
            , stddev_steal_amt_()
            , single_steal_fails_ (0)
            , session_steal_successes_ (0)
            , session_steal_fails_ (0)
            , acquire_successes_ (0)
            , acquire_fails_ (0)
            , releases_ (0)
            , public_tasks_dequeued_ (0)
            , private_tasks_dequeued_ (0)
            , remote_private_tasks_spawned_ (0)

            , sample_calls (0)
#ifdef VTRACE_SAMPLED
              , task_manager_vt_grp( VT_COUNT_GROUP_DEF( "Task manager" ) )
              , privateQ_size_vt_ev( VT_COUNT_DEF( "privateQ size", "tasks", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
              , publicQ_size_vt_ev( VT_COUNT_DEF( "publicQ size", "tasks", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
              , session_steal_successes_vt_ev( VT_COUNT_DEF( "session_steal_successes", "steals", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
              , session_steal_fails_vt_ev( VT_COUNT_DEF( "session_steal_fails", "steals", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
              , single_steal_successes_vt_ev( VT_COUNT_DEF( "single_steal_successes", "steals", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
              , total_steal_tasks_vt_ev( VT_COUNT_DEF( "total_steal_tasks", "steals", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
              , single_steal_fails_vt_ev( VT_COUNT_DEF( "single_steal_fails", "steals", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
              , acquire_successes_vt_ev( VT_COUNT_DEF( "acquire_successes", "acquires", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
              , acquire_fails_vt_ev( VT_COUNT_DEF( "acquire_fails", "acquires", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
              , releases_vt_ev( VT_COUNT_DEF( "releases", "acquires", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
              , public_tasks_dequeued_vt_ev( VT_COUNT_DEF( "public_tasks_dequeued", "tasks", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
              , private_tasks_dequeued_vt_ev( VT_COUNT_DEF( "private_tasks_dequeued", "tasks", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
              , remote_private_tasks_spawned_vt_ev ( VT_COUNT_DEF( "remote_private_tasks_spawned", "tasks", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
#endif

              , tm( task_manager )
              { }

        void sample();
        void profiling_sample();

        void record_successful_steal_session() {
          session_steal_successes_++;
        }

        void record_failed_steal_session() {
          session_steal_fails_++;
        }

        void record_successful_steal( int64_t amount ) {
          single_steal_successes_++;
          total_steal_tasks_+=amount;
          max_steal_amt_ = max2( max_steal_amt_, amount );
          stddev_steal_amt_.addSample( amount );
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

        void record_remote_private_task_spawn() {
          remote_private_tasks_spawned_++;
        }

        void dump( std::ostream& o, const char * terminator );
        void merge(TaskStatistics * other);
        void reset();

        static void merge_am(TaskManager::TaskStatistics * other, size_t sz, void* payload, size_t psz);

    };

    /// task statistics object
    TaskStatistics stats;

    TaskManager();
    void init (bool doSteal, Node localId, Node* neighbors, Node numLocalNodes, int chunkSize);

    /// @return true if work is considered finished and
    ///         the task system is terminating
    bool isWorkDone() {
      return workDone;
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

    void dump_stats( std::ostream& o, const char * terminator );
    void dump_stats();
    void merge_stats();
    void reset_stats();
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
           || (doSteal && stealLock );
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
  publicQ.push( newtask );
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
  stats.record_remote_private_task_spawn();
  /// note from cbarrier implementation
  /*
   * local cancel cbarrier
   */
}

/// system task manager
extern TaskManager global_task_manager;

#endif // TASK_HPP
