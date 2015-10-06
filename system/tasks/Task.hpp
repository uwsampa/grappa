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
#ifndef TASK_HPP
#define TASK_HPP

#include <iostream>
#include <deque>
#include "Worker.hpp"

#define PRIVATEQ_LIFO 1


namespace Grappa {
  namespace impl {

// forward declaration of Grappa Core
typedef int16_t Core;

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

class TaskManagerMetrics {
  public:
    static void record_successful_steal_session();
    static void record_failed_steal_session();
    static void record_successful_steal( int64_t amount );
    static void record_failed_steal();
    static void record_successful_acquire();
    static void record_failed_acquire();
    static void record_release();
    static void record_public_task_dequeue();
    static void record_private_task_dequeue();
    static void record_globalq_push( uint64_t amount, bool success );
    static void record_globalq_pull_start( );
    static void record_globalq_pull( uint64_t amount );
    static void record_workshare_test();
    static void record_remote_private_task_spawn();
    static void record_workshare( int64_t change );
};

/// Keeps track of tasks, pairing workers with tasks, and load balancing.
class TaskManager {
  private:
    /// queue for tasks assigned specifically to this Core
    std::deque<Task> privateQ; 

    /// indicates that all tasks *should* be finished
    /// and termination can occur
    bool workDone;


    /// machine-local id (to support hierarchical dynamic load balancing)
    Core localId;

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
    Core* neighbors;

    /// number of Cores on local machine (to support hierarchical dynamic load balancing)
    Core numLocalNodes;

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

    /// @return true if Core-private queue has elements
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


    //TaskManager (bool doSteal, Core localId, Core* neighbors, Core numLocalNodes, int chunkSize, int cbint);
    TaskManager();
    void init (Core localId, Core* neighbors, Core numLocalNodes);
    void activate();
  
    size_t estimate_footprint() const;
    size_t adjust_footprint(size_t target);
    
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

    uint64_t numLocalPublicTasks() const;
    uint64_t numLocalPrivateTasks() const;

    bool getWork ( Task * result );

    bool available ( ) const;
    bool local_available ( ) const;

    void finish();

    friend std::ostream& operator<<( std::ostream& o, const TaskManager& tm );

    void signal_termination( );
};

/// Whether work possibly exists locally or globally
inline bool TaskManager::available( ) const {
  DVLOG(6) << " publicHasEle()=" << publicHasEle()
    << " privateHasEle()=" << privateHasEle();
  return privateHasEle() 
    || publicHasEle()
    || (doSteal && stealLock )
    || (doShare && wshareLock )
    || (doGQ    && gqPullLock );
}

/// Whether work exists locally
inline bool TaskManager::local_available( ) const {
  DVLOG(6) << " publicHasEle()=" << publicHasEle()
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
#if PRIVATEQ_LIFO
  privateQ.push_front( newtask );
#else
  privateQ.push_back( newtask );
#endif

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
#if PRIVATEQ_LIFO
  privateQ.push_front( newtask );
#else
  privateQ.push_back( newtask );
#endif
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
