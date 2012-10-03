#ifndef TASK_HPP_
#define TASK_HPP_

#include <iostream>
#include <deque>
#include "StealQueue.hpp"
#include "cbarrier.hpp"
#include "Thread.hpp"
#include "StatisticsTools.hpp"

#ifdef VTRACE
#include <vt_user.h>
#endif
#ifdef VTRACE_SAMPLED
#include <vt_user.h>
#endif

#define publicQ StealQueue<Task>::steal_queue

typedef int16_t Node;

/// Task is a function pointer and pointer to arguments
/// Ideally Task would be interface that just declares execute and makeGlobal
class Task {

  private:
    void (* fn_p)(void*,void*,void*);
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
    Task () {}
    Task (void (* fn_p)(void*, void*, void*), void* arg0, void* arg1, void* arg2) 
      : fn_p ( fn_p )
        , arg0 ( arg0 )
        , arg1 ( arg1 )
        , arg2 ( arg2 ) { }

    void execute( ) {
      CHECK( fn_p!=NULL ) << "fn_p=" << (void*)fn_p << "\narg0=" << (void*)arg0 << "\narg1=" << (void*)arg1 << "\narg2=" << (void*)arg2;
      fn_p( arg0, arg1, arg2 );  // NOTE: this executes 1-parameter function's with 3 args
    }

    friend std::ostream& operator<<( std::ostream& o, const Task& t );
};

template < typename T, typename S, typename R >
static Task createTask( void (*fn_p)(T, S, R), T arg0, S arg1, R arg2 ) {
  Task t( reinterpret_cast< void (*) (void*, void*, void*) >( fn_p ), (void*)arg0, (void*)arg1, (void*)arg2 );
  return t;
}

 
class TaskManager {
    private:
        std::deque<Task> privateQ; 
        /* global steal_queue StealQueue<Task> publicQ; */

        bool workDone;
       
        bool all_terminate;

        bool doSteal;    // stealing on/off
        bool doShare;    // sharing on/off
        bool doGQ;       // global queue on/off
        bool stealLock;  // steal lock
        bool wshareLock; // work share lock
        bool gqPushLock;     // global queue push lock
        bool gqPullLock;     // global queue pull lock

        // to support hierarchical dynamic load balancing
        Node localId;
        Node* neighbors;
        Node numLocalNodes;
        int64_t nextVictimIndex;

        // steal parameters
        int chunkSize;
        
        /// Flags to save whether a worker thinks there
        /// could be work or if other workers should not
        /// also try.
        bool sharedMayHaveWork;
       
        bool publicHasEle() const {
            return publicQ.depth() > 0;
        }

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
        
        
        std::ostream& dump( std::ostream& o ) const {
            return o << "TaskManager {" << std::endl
                << "  publicQ: " << publicQ.depth( ) << std::endl
                << "  privateQ: " << privateQ.size() << std::endl
                << "  work-may-be-available? " << available() << std::endl
                << "  sharedMayHaveWork: " << sharedMayHaveWork << std::endl
                << "  workDone: " << workDone << std::endl
                << "  stealLock: " << stealLock << std::endl
                << "  wshareLock: " << wshareLock << std::endl
                << "}";
        }



    public:
        class TaskStatistics {
            private:
                uint64_t single_steal_successes_;
                TotalStatistic steal_amt_;
                uint64_t single_steal_fails_;
                uint64_t session_steal_successes_;
                uint64_t session_steal_fails_;
                uint64_t acquire_successes_;
                uint64_t acquire_fails_;
                uint64_t releases_;
                uint64_t public_tasks_dequeued_;
                uint64_t private_tasks_dequeued_;

                uint64_t globalq_pushes_;
                uint64_t globalq_push_attempts_;
                TotalStatistic globalq_elements_pushed_;
                uint64_t globalq_pulls_;
                uint64_t globalq_pull_attempts_;
                TotalStatistic globalq_elements_pulled_;

                uint64_t workshare_tests_;
                uint64_t workshares_initiated_;
                TotalStatistic workshares_initiated_received_elements_;
                TotalStatistic workshares_initiated_pushed_elements_;

                // number of calls to sample() 
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

    unsigned globalq_pushes_vt_ev;
    unsigned globalq_push_attempts_vt_ev;
    unsigned globalq_elements_pushed_vt_ev;
    unsigned globalq_pulls_vt_ev;
    unsigned globalq_pull_attempts_vt_ev;
    unsigned globalq_elements_pulled_vt_ev;

    unsigned shares_initiated_vt_ev;
    unsigned shares_received_elements_vt_ev;
    unsigned shares_pushed_elements_vt_ev;
#endif

                TaskManager * tm;

            public:
                TaskStatistics() { } // only for declarations that will be copy-assigned to

                TaskStatistics(TaskManager * task_manager)
                      : steal_amt_ ()
                      , globalq_elements_pushed_ ()
                      , workshares_initiated_received_elements_ ()
                      , workshares_initiated_pushed_elements_ ()
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
       
        , globalq_pushes_vt_ev( VT_COUNT_DEF( "globalq pushes", "tasks", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
        , globalq_push_attempts_vt_ev( VT_COUNT_DEF( "globalq push attempts", "tasks", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
        , globalq_elements_pushed_vt_ev( VT_COUNT_DEF( "globalq elements pushed", "tasks", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
        , globalq_pulls_vt_ev( VT_COUNT_DEF( "globalq pulls", "tasks", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
        , globalq_pull_attempts_vt_ev( VT_COUNT_DEF( "globalq pull attempts", "tasks", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
        , globalq_elements_pulled_vt_ev( VT_COUNT_DEF( "globalq elements pulled", "tasks", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
        
        , shares_initiated_vt_ev( VT_COUNT_DEF( "workshares initiated", "tasks", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
        , shares_received_elements_vt_ev( VT_COUNT_DEF( "workshares received elements", "tasks", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
        , shares_pushed_elements_vt_ev( VT_COUNT_DEF( "workshares pushed elements", "tasks", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
#endif

                      , tm( task_manager )
                          { reset(); }

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
                    steal_amt_.update( amount );
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
                    globalq_elements_pushed_.update(amount);
                    globalq_pushes_ += 1;
                  }
                }

                void record_globalq_pull_start( ) {
                  globalq_pull_attempts_ += 1;
                }

                void record_globalq_pull( uint64_t amount ) {
                  if ( amount > 0 ) {
                    globalq_elements_pulled_.update(amount);
                    globalq_pulls_ += 1;
                  }
                }

                void record_workshare_test() {
                  workshare_tests_++;
                }

                void record_workshare( int64_t change ) {
                  workshares_initiated_ += 1;
                  if ( change < 0 ) {
                    workshares_initiated_pushed_elements_.update((-change));
                  } else {
                    workshares_initiated_received_elements_.update( change );
                  }
                }

                void dump();
                void merge(const TaskStatistics * other);
                void reset();
        };
        
        TaskStatistics stats;
  
        //TaskManager (bool doSteal, Node localId, Node* neighbors, Node numLocalNodes, int chunkSize, int cbint);
        TaskManager();
        void init (Node localId, Node* neighbors, Node numLocalNodes);

        bool isWorkDone() {
            return workDone;
        }

        bool global_queue_on() {
          return doGQ;
        }
        
    /*TODO return value?*/
    template < typename T, typename S, typename R > 
      void spawnPublic( void (*f)(T, S, R), T arg0, S arg1, R arg2 );

    /*TODO return value?*/ 
    template < typename T, typename S, typename R > 
      void spawnLocalPrivate( void (*f)(T, S, R), T arg0, S arg1, R arg2 );

    /*TODO return value?*/ 
    template < typename T, typename S, typename R > 
      void spawnRemotePrivate( void (*f)(T, S, R), T arg0, S arg1, R arg2 );

        bool getWork ( Task * result );

        bool available ( ) const;
        bool local_available ( ) const;
        
        void dump_stats();
        void merge_stats();
        void reset_stats();
        void finish();
    
        friend std::ostream& operator<<( std::ostream& o, const TaskManager& tm );

     void signal_termination( );
};


inline bool TaskManager::available( ) const {
    VLOG(6) << " publicHasEle()=" << publicHasEle()
            << " privateHasEle()=" << privateHasEle();
    return privateHasEle() 
           || publicHasEle()
           || (doSteal && stealLock )
           || (doShare && wshareLock )
           || (doGQ    && gqPullLock );
}

inline bool TaskManager::local_available( ) const {
    VLOG(6) << " publicHasEle()=" << publicHasEle()
            << " privateHasEle()=" << privateHasEle();
    return privateHasEle() 
           || publicHasEle();
}


template < typename T, typename S, typename R > 
inline void TaskManager::spawnPublic( void (*f)(T, S, R), T arg0, S arg1, R arg2 ) {
  Task newtask = createTask(f, arg0, arg1, arg2 );
  publicQ.push( newtask );

}

/// Should NOT be called from the context of
/// an AM handler
template < typename T, typename S, typename R >
inline void TaskManager::spawnLocalPrivate( void (*f)(T, S, R), T arg0, S arg1, R arg2 ) {
  Task newtask = createTask( f, arg0, arg1, arg2 );
  privateQ.push_back( newtask );

  /// note from cbarrier implementation
    /* no notification necessary since
     * presence of a local spawn means
     * we are not in the cbarrier */
}

/// Should ONLY be called from the context of
/// an AM handler
template < typename T, typename S, typename R > 
inline void TaskManager::spawnRemotePrivate( void (*f)(T, S, R), T arg0, S arg1, R arg2 ) {
  Task newtask = createTask( f, arg0, arg1, arg2 );
  privateQ.push_front( newtask );

  /// note from cbarrier implementation
  /*
   * local cancel cbarrier
   */
}

extern TaskManager global_task_manager;

#endif
