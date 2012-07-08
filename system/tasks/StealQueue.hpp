
#ifndef STEAL_QUEUE_HPP
#define STEAL_QUEUE_HPP

#include <iostream>
#include <glog/logging.h>   
#include <stdlib.h>

// profiling/tracing
#include "../PerformanceTools.hpp"

#ifdef VTRACE
#include <vt_user.h>
#endif

#define MIN_INT(a, b) ( (a) < (b) ) ? (a) : (b);

#define SS_NSTATES 1

struct workStealRequest_args;
struct workStealReply_args;

/// Type for Node ID. 
#include <boost/cstdint.hpp>
typedef int16_t Node;

/// Forward declare for steal_locally
class Thread;


template <typename T>
class StealQueue {
    private:
        uint64_t stackSize;     /* total space avail (in number of elements) */
        uint64_t workAvail;     /* elements available for stealing */
        uint64_t bottom;   /* index of start of shared portion of stack */
        uint64_t top;           /* index of stack top */
        uint64_t maxStackDepth;                      /* stack stats */ 
        uint64_t nNodes, maxTreeDepth, nVisited, nLeaves;        /* tree stats: (num pushed, max depth, num popped, leaves)  */
        uint64_t nAcquire, nRelease, nStealPackets, nFail;  /* steal stats */
        uint64_t wakeups, falseWakeups, nNodes_last;
  
#ifdef VTRACE
  unsigned steal_queue_grp_vt;
  unsigned steal_success_ev_vt;
  unsigned steal_victim_ev_vt;
#endif

        double time[SS_NSTATES], timeLast;
        /* perf measurements */ 
        int entries[SS_NSTATES], curState; 
        /* lock for manipulation of shared portion */ 
        T* stack;       /* addr of actual
                                  stack of nodes
                                  in local addr
                                  space */
        T* stack_g; /* addr of same
                              stack in global
                              addr space */

        static StealQueue<T>* staticQueueAddress;        
        
        void steal_reply( uint64_t amt, uint64_t total, T * stolen_work, size_t stolen_size_bytes );
        static void workStealReply_am( workStealReply_args * args,  size_t size, void * payload, size_t payload_size );
        static void workStealRequest_am( workStealRequest_args * args, size_t size, void * payload, size_t payload_size );
        
        std::ostream& dump ( std::ostream& o ) const {
            return o << "StealQueue[depth=" << depth()
                     << "; indices(top= " << top 
                              << " bottom=" << bottom << ")"
                     << "; stackSize=" << stackSize << "]";
        }
    
    public:
        StealQueue( uint64_t numEle ) 
            : stackSize( numEle )
            , maxStackDepth( 0 )
            , nNodes( 0 ), maxTreeDepth( 0 ), nVisited( 0 ), nLeaves( 0 )
            , nAcquire( 0 ), nRelease( 0 ), nStealPackets( 0 ), nFail( 0 )
            , wakeups( 0 ), falseWakeups( 0 ), nNodes_last( 0 ) 
#ifdef VTRACE
	    , steal_queue_grp_vt( VT_COUNT_GROUP_DEF( "Steal queue" ) )
	    , steal_success_ev_vt( VT_COUNT_DEF( "Steal success", "tasks", VT_COUNT_TYPE_UNSIGNED, steal_queue_grp_vt ) )
	    , steal_victim_ev_vt( VT_COUNT_DEF( "Steal victim", "tasks", VT_COUNT_TYPE_UNSIGNED, steal_queue_grp_vt ) )
#endif
{

                uint64_t nbytes = numEle * sizeof(T);

                // allocate stack in shared addr space with affinity to calling thread
                // and record local addr for efficient access in sequel
                stack_g = static_cast<T*>( malloc( nbytes ) );
                stack = stack_g;

                CHECK( stack!= NULL ) << "Request for " << nbytes << " bytes for stealStack failed";

                mkEmpty();
            }
        
        void mkEmpty(); 
        void push( T c); 
        T peek( ); 
        void pop( ); 
        uint64_t topPosn( ) const;
        uint64_t depth( ) const; 
        void release( int k ); 
        int acquire( int k ); 
        int steal_locally( Node victim, int chunkSize ); 
        void setState( int state );
        
        uint64_t get_nNodes( ) {
            return nNodes;
        }
        
        /// register local address of remote steal queues
        static void registerAddress( StealQueue<T> * addr );

        template< typename U >
        friend std::ostream& operator<<( std::ostream& o, const StealQueue<U>& sq );
};


static int maxint(int x, int y) { return (x>y)?x:y; }

/// Push onto top of local stack
template <typename T>
inline void StealQueue<T>::push( T c ) {
  CHECK( top < stackSize ) << "push: overflow (top:" << top << " stackSize:" << stackSize << ")";

  stack[top] = c; 
  top++;
  nNodes++;
  maxStackDepth = maxint(top, maxStackDepth);
  //s->maxTreeDepth = maxint(s->maxTreeDepth, c->height); //XXX dont want to deref c here (expensive for just a bookkeeping operation
}

/// get top element
template <typename T>
inline T StealQueue<T>::peek( ) {
    CHECK(top > bottom) << "peek: empty local stack";
    return stack[top-1];
}

/// local pop
template <typename T>
inline void StealQueue<T>::pop( ) {
  CHECK(top > bottom) << "pop: empty local stack";
  
#if DEBUG
  // 0 out the popped element (to detect errors)
  memset( &stack[top-1], 0, sizeof(T) );
#endif
  
  top--;
  nVisited++;
}


/// depth
template <typename T>
inline uint64_t StealQueue<T>::depth() const {
  return (top - bottom);
}


template <typename T>
void StealQueue<T>::setState( int state ) { return; }


/// Initialize the dedicated queue for T
template <typename T>
StealQueue<T>* StealQueue<T>::staticQueueAddress = NULL;

template <typename T>
void StealQueue<T>::registerAddress( StealQueue<T> * addr ) {
    staticQueueAddress = addr;
}

/// set queue to empty
template <typename T>
inline void StealQueue<T>::mkEmpty( ) {
    bottom = 0;
    top    = 0;
}

/// local top position:  stack index of top element
template <typename T>
uint64_t StealQueue<T>::topPosn() const
{
  CHECK ( top > bottom ) << "ss_topPosn: empty local stack";
  return top - 1;
}


//////////////////////////////////////////////////
// Work stealing
/////////////////////////////////////////////////

//#include "../SoftXMT.hpp" 
#include <Communicator.hpp>
#include <tasks/TaskingScheduler.hpp>

extern TaskingScheduler global_scheduler;
// void SoftXMT_suspend();
// void SoftXMT_wake( Thread * );
// Node SoftXMT_mynode();

struct workStealRequest_args {
    int k;
    Node from;
};

struct workStealReply_args {
    int stealAmt;
    int total;
};

static int64_t local_steal_amount;
static uint64_t received_tasks;
static Thread * steal_waiter = NULL;

template <typename T>
void StealQueue<T>::steal_reply( uint64_t amt, uint64_t total, T * stolen_work, size_t stolen_size_bytes ) {
    if (amt > 0) {
      GRAPPA_EVENT(steal_packet_ev, "Steal packet", 1, GRAPPA_SCHEDULER_GROUP, amt);

#ifdef VTRACE
      //VT_COUNT_UNSIGNED_VAL( thiefStack->steal_success_ev_vt, k );
#endif

      // reclaim array space if empty
      if ( depth() == 0 ) {
        mkEmpty();
      }

      CHECK( top + amt < stackSize ) << "steal reply: overflow (top:" << top << " stackSize:" << stackSize << " amt:" << amt << ")";
      memcpy(&stack[top], stolen_work, stolen_size_bytes);

      received_tasks += amt;
      VLOG(5) << "Steal packet returns with amt=" << amt << ", received=" << received_tasks << " / total=" << total;
      top += amt;
      nStealPackets++;

      /// The steal requestor stays asleep until all received, but other threads can take advantage
      /// of the tasks that have been copied in
      if ( received_tasks == total ) { 
        GRAPPA_EVENT(steal_success_ev, "Steal success", 1, GRAPPA_SCHEDULER_GROUP, total);
        VLOG(5) << "Last packet; will wake steal_waiter=" << steal_waiter;
        local_steal_amount = total;
        if ( steal_waiter != NULL ) {
          //SoftXMT_wake( steal_waiter );
          global_scheduler.thread_wake( steal_waiter );
          steal_waiter = NULL;
        }
      }

    } else {
      local_steal_amount = 0;
      nFail++;
      
      if ( steal_waiter != NULL ) {
        //SoftXMT_wake( steal_waiter );
        global_scheduler.thread_wake( steal_waiter );
        steal_waiter = NULL;
      }
    }
}

template <typename T>
void StealQueue<T>::workStealReply_am( workStealReply_args * args,  size_t size, void * payload, size_t payload_size ) {
    CHECK ( local_steal_amount == -1 ) << "local_steal_amount=" << local_steal_amount << " when steal reply arrives";
    CHECK( args->stealAmt * sizeof(T) == payload_size ) << "steal amount in bytes != payload size";

    T * stolen_work = static_cast<T*>( payload );
    
    StealQueue<T>* thiefStack = StealQueue<T>::staticQueueAddress;
    thiefStack->steal_reply( args->stealAmt, args->total, stolen_work, payload_size );
}


template <typename T>
void StealQueue<T>::workStealRequest_am(workStealRequest_args * args, size_t size, void * payload, size_t payload_size) {
    int k = args->k;

    StealQueue<T>* victimStack = StealQueue<T>::staticQueueAddress;
   
    int victimTop = victimStack->top;
    int victimBottom = victimStack->bottom;
    
    const int victimHalfWorkAvail = (victimTop - victimBottom) / 2;
    const int stealAmt = MIN_INT( victimHalfWorkAvail, k );
    bool ok = stealAmt > 0;
  
    VLOG(4) << "Victim of thief=" << args->from << " victimHalfWorkAvail=" << victimHalfWorkAvail;
    if (ok) {

      /* reserve a chunk */
      victimStack->bottom =  victimBottom + stealAmt;


      GRAPPA_EVENT(steal_victim_ev, "Steal victim", 1, scheduler, stealAmt);
#ifdef VTRACE
      //VT_COUNT_UNSIGNED_VAL( victimStack->steal_victim_ev_vt, k );
#endif

      T* victimStackBase = victimStack->stack;
      T* victimStealStart = victimStackBase + victimBottom;

      const int bufsize = 110; // TODO: I now 32B*110 < aggreg bufsize-header size
                               // but do this precisely
      int offset = 0;
      for ( int remain = stealAmt; remain > 0; ) {
        int transfer_amt = (remain < bufsize) ? remain : bufsize;
        VLOG(5) << "sending steal packet of transfer_amt=" << transfer_amt << " remain=" << remain << " / stealAmt=" << stealAmt;
        remain -= transfer_amt;
        workStealReply_args reply_args = { transfer_amt, stealAmt };
        SoftXMT_call_on( args->from, &StealQueue<T>::workStealReply_am, 
            &reply_args, sizeof(workStealReply_args), 
            victimStealStart + offset, transfer_amt*sizeof( T ));

        offset += transfer_amt;
      }

#if DEBUG
      // 0 out the stolen stuff (to detect errors)
      memset( victimStealStart, 0, stealAmt*sizeof( T ) );
#endif

    } else {
      workStealReply_args reply_args = { 0, 0 };
      SoftXMT_call_on( args->from, &StealQueue<T>::workStealReply_am, &reply_args );
    }

}

#include "Thread.hpp"
template <typename T>
int StealQueue<T>::steal_locally( Node victim, int op ) {

    // initialize stealing state
    local_steal_amount = -1;
    received_tasks = 0;

    workStealRequest_args req_args = { op, global_communicator.mynode() };
    SoftXMT_call_on( victim, &StealQueue<T>::workStealRequest_am, &req_args );

    GRAPPA_PROFILE_CREATE( stealprof, "steal_locally", "(suspended)", GRAPPA_SUSPEND_GROUP );
        
    // reclaim array space if empty
    if ( depth() == 0 ) {
      mkEmpty();
    }

    // steal is blocking
    // TODO: use suspend-wake mechanism
    while ( local_steal_amount == -1 ) {
        steal_waiter = global_scheduler.get_current_thread();
        
        GRAPPA_PROFILE_THREAD_START( stealprof, global_scheduler.get_current_thread() );
	    
        global_scheduler.thread_suspend();
        //SoftXMT_suspend();
        
        GRAPPA_PROFILE_THREAD_STOP( stealprof, global_scheduler.get_current_thread() );
    }

    return local_steal_amount;
}
/////////////////////////////////////////////////////////

template <typename T>
std::ostream& operator<<( std::ostream& o, const StealQueue<T>& sq ) {
    return sq.dump( o );
}

#endif
