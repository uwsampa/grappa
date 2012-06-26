#ifndef TASK_HPP_
#define TASK_HPP_

#include <iostream>
#include <deque>
#include "StealQueue.hpp"
#include "cbarrier.hpp"
#include "Thread.hpp"

#ifdef VTRACE
#include <vt_user.h>
#endif
#ifdef VTRACE_SAMPLED
#include <vt_user.h>
#endif

typedef int16_t Node;

/// Task is a function pointer and pointer to arguments
/// Ideally Task would be interface that just declares execute and makeGlobal
class Task {

  private:
    void (* fn_p)(void*,void*);
    void* arg;
    void* shared_arg;

  public:
    Task () {}
    Task (void (* fn_p)(void*, void*), void* arg, void* shared_arg) 
      : fn_p ( fn_p )
        , arg ( arg )
        , shared_arg( shared_arg ) { }

    void execute( ) {
      CHECK( fn_p!=NULL ) << "fn_p=" << (void*)fn_p << "\narg=" << (void*)arg << "\nshared_arg=" << (void*)shared_arg;
      fn_p( arg, shared_arg );
    }
};

template < typename T, typename S>
static Task createTask( void (*fn_p)(T, S), T arg, S shared_arg ) {
  Task t( reinterpret_cast< void (*) (void*, void*) >( fn_p ), (void*)arg, (void*)shared_arg);
  return t;
}

 
class TaskManager {
    private:
        std::deque<Task> privateQ; 
        StealQueue<Task> publicQ;

        bool workDone;
       
        bool all_terminate;

        bool doSteal;   // stealing on/off
        bool stealLock; // steal lock
        int cbint;      // how often to make local public work visible

        // to support hierarchical dynamic load balancing
        Node localId;
        Node* neighbors;
        Node numLocalNodes;

        // steal parameters
        int chunkSize;
        
        /// Flags to save whether a worker thinks there
        /// could be work or if other workers should not
        /// also try.
        bool sharedMayHaveWork;
       
        bool publicHasEle() const {
            return publicQ.localDepth() > 0;
        }

        bool privateHasEle() const {
            return !privateQ.empty();
        }
        
        // "queue" operations
        bool tryConsumeLocal( Task * result );
        bool tryConsumeShared( Task * result );
        bool waitConsumeAny( Task * result );
        
        
        
        std::ostream& dump( std::ostream& o ) const {
            return o << "TaskManager {" << std::endl
                << "  publicQ.local: " << publicQ.localDepth( ) << std::endl
                << "  publicQ.shared: " << publicQ.sharedDepth( ) << std::endl
                << "  privateQ: " << privateQ.size() << std::endl
                << "  work-may-be-available? " << available() << std::endl
                << "  sharedMayHaveWork: " << sharedMayHaveWork << std::endl
                << "  workDone: " << workDone << std::endl
                << "  stealLock: " << stealLock << std::endl
                << "}";
        }



    public:
        class TaskStatistics {
            private:
                uint64_t single_steal_successes_;
                uint64_t single_steal_fails_;
                uint64_t session_steal_successes_;
                uint64_t session_steal_fails_;
                uint64_t acquire_successes_;
                uint64_t acquire_fails_;
                uint64_t releases_;
                uint64_t public_tasks_dequeued_;
                uint64_t private_tasks_dequeued_;

                // number of calls to sample() 
                uint64_t sample_calls;

#ifdef VTRACE_SAMPLED
	  unsigned task_manager_vt_grp;
	  unsigned privateQ_size_vt_ev;
	  unsigned publicQ_local_size_vt_ev;
	  unsigned publicQ_shared_size_vt_ev;
	  unsigned session_steal_successes_vt_ev;
	  unsigned session_steal_fails_vt_ev;
	  unsigned single_steal_successes_vt_ev;
	  unsigned single_steal_fails_vt_ev;
	  unsigned acquire_successes_vt_ev;
	  unsigned acquire_fails_vt_ev;
	  unsigned releases_vt_ev;
	  unsigned public_tasks_dequeued_vt_ev;
	  unsigned private_tasks_dequeued_vt_ev;
#endif

                TaskManager * tm;

            public:
                TaskStatistics(TaskManager * task_manager)
                    : single_steal_successes_ (0)
                      , single_steal_fails_ (0)
                      , session_steal_successes_ (0)
                      , session_steal_fails_ (0)
                      , acquire_successes_ (0)
                      , acquire_fails_ (0)
                      , releases_ (0)
                      , public_tasks_dequeued_ (0)
                      , private_tasks_dequeued_ (0)

                      , sample_calls (0)
#ifdef VTRACE_SAMPLED
		    , task_manager_vt_grp( VT_COUNT_GROUP_DEF( "Task manager" ) )
		    , privateQ_size_vt_ev( VT_COUNT_DEF( "privateQ size", "tasks", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
		    , publicQ_local_size_vt_ev( VT_COUNT_DEF( "publicQ local size", "tasks", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
		    , publicQ_shared_size_vt_ev( VT_COUNT_DEF( "publicQ shared size", "tasks", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
		    , session_steal_successes_vt_ev( VT_COUNT_DEF( "session_steal_successes", "steals", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
		    , session_steal_fails_vt_ev( VT_COUNT_DEF( "session_steal_fails", "steals", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
		    , single_steal_successes_vt_ev( VT_COUNT_DEF( "single_steal_successes", "steals", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
		    , single_steal_fails_vt_ev( VT_COUNT_DEF( "single_steal_fails", "steals", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
		    , acquire_successes_vt_ev( VT_COUNT_DEF( "acquire_successes", "acquires", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
		    , acquire_fails_vt_ev( VT_COUNT_DEF( "acquire_fails", "acquires", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
		    , releases_vt_ev( VT_COUNT_DEF( "releases", "acquires", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
		    , public_tasks_dequeued_vt_ev( VT_COUNT_DEF( "public_tasks_dequeued", "tasks", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
		    , private_tasks_dequeued_vt_ev( VT_COUNT_DEF( "private_tasks_dequeued", "tasks", VT_COUNT_TYPE_UNSIGNED, task_manager_vt_grp ) )
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

                void record_successful_steal() {
                    single_steal_successes_++;
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

                void dump();
                void merge(TaskStatistics * other);
                static void merge_am(TaskManager::TaskStatistics * other, size_t sz, void* payload, size_t psz);

        };
        
        TaskStatistics stats;
  
        //TaskManager (bool doSteal, Node localId, Node* neighbors, Node numLocalNodes, int chunkSize, int cbint);
        TaskManager();
        void init (bool doSteal, Node localId, Node* neighbors, Node numLocalNodes, int chunkSize, int cbint);

        bool isWorkDone() {
            return workDone;
        }
        
        /// Maybe release tasks to the shared portion of publicQ
        void releaseTasks() {
            if (doSteal) {
                if (publicQ.localDepth() > 2 * chunkSize) {
                    publicQ.release(chunkSize);
                    stats.record_release(); 

                    // set that there COULD be work in shared portion
                    // (not "is work" because may be stolen)
                    sharedMayHaveWork = true;
                }
            }
        }

    /*TODO return value?*/
    template < typename T, typename S > 
      void spawnPublic( void (*f)(T, S), T arg, S shared_arg);

    /*TODO return value?*/ 
    template < typename T, typename S > 
      void spawnLocalPrivate( void (*f)(T, S), T arg, S shared_arg);

    /*TODO return value?*/ 
    template < typename T, typename S > 
      void spawnRemotePrivate( void (*f)(T, S), T arg, S shared_arg);

        bool getWork ( Task * result );

        bool available ( ) const;
        bool local_available ( ) const;
        
        void dump_stats();
        void merge_stats();
        void finish();
    
        friend std::ostream& operator<<( std::ostream& o, const TaskManager& tm );

     void signal_termination( );
};


inline bool TaskManager::available( ) const {
    VLOG(6) << " sharedMayHaveWork=" << sharedMayHaveWork
            << " publicHasEle()=" << publicHasEle()
            << " privateHasEle()=" << privateHasEle();
    return privateHasEle() 
           || publicHasEle()
           || (doSteal && (sharedMayHaveWork || stealLock ));
}

inline bool TaskManager::local_available( ) const {
    VLOG(6) << " sharedMayHaveWork=" << sharedMayHaveWork
            << " publicHasEle()=" << publicHasEle()
            << " privateHasEle()=" << privateHasEle();
    return privateHasEle() 
           || publicHasEle()
           || (doSteal && sharedMayHaveWork);
}


template < typename T, typename S > 
inline void TaskManager::spawnPublic( void (*f)(T, S), T arg, S shared_arg) {
  Task newtask = createTask(f, arg, shared_arg);
  publicQ.push( newtask );
  releaseTasks();
}

/// Should NOT be called from the context of
/// an AM handler
template < typename T, typename S >
inline void TaskManager::spawnLocalPrivate( void (*f)(T, S), T arg, S shared_arg) {
  Task newtask = createTask(f, arg, shared_arg);
  privateQ.push_back( newtask );

    /* no notification necessary since
     * presence of a local spawn means
     * we are not in the cbarrier */
}

/// Should ONLY be called from the context of
/// an AM handler
template < typename T, typename S > 
inline void TaskManager::spawnRemotePrivate( void (*f)(T, S), T arg, S shared_arg ) {
  Task newtask = createTask(f, arg, shared_arg);
  privateQ.push_front( newtask );

  cbarrier_cancel_local();
}


extern TaskManager global_task_manager;

#endif
