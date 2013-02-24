// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.


#ifndef STEALQUEUE_HPP
#define STEALQUEUE_HPP

#include <iostream>
#include <glog/logging.h>   
#include <gflags/gflags.h>
#include <stdlib.h>

#include <sstream>

// profiling/tracing
#include "../PerformanceTools.hpp"
#include "../StatisticsTools.hpp"

#ifdef VTRACE
#include <vt_user.h>
#endif


#include "../Addressing.hpp"
#include "../LegacySignaler.hpp"


// forward declaration 
size_t Grappa_sizeof_header();


// global queue forward declarations
template <typename T>
struct ChunkInfo;

template <typename T>
void global_queue_pull( ChunkInfo<T> * result );
template <typename T>
bool global_queue_push( GlobalAddress<T> chunk_base, uint64_t chunk_amount );



#define MIN_INT(a, b) ( (a) < (b) ) ? (a) : (b)

GRAPPA_DECLARE_EVENT_GROUP(scheduler);

#define SS_NSTATES 1

struct workStealRequest_args;
struct workStealReply_args;
struct workShareRequest_args;
struct workShareReply_args;

template <typename T>
struct pull_global_data_args {
  GlobalAddress<Signaler> signal;
  ChunkInfo<T> chunk;
};


/// Type for Node ID. 
#include <boost/cstdint.hpp>
typedef int16_t Node;

/// Forward declare for steal_locally
class Thread;


class StealStatistics {
  private:
    // work steal network usage
    uint64_t stealq_reply_messages;
    uint64_t stealq_reply_total_bytes;
    uint64_t stealq_request_messages;
    uint64_t stealq_request_total_bytes;

    // work share network usage 
    uint64_t workshare_request_messages;
    uint64_t workshare_request_total_bytes;
    uint64_t workshare_reply_messages;
    uint64_t workshare_reply_total_bytes;

    // work share elements transfered
    TotalStatistic workshare_request_elements_denied;
    TotalStatistic workshare_request_elements_received;
    TotalStatistic workshare_reply_elements_sent;
    uint64_t workshare_requests_client_smaller;
    uint64_t workshare_requests_client_larger;
    uint64_t workshare_reply_nacks;

    // global queue data transfer network usage
    uint64_t globalq_data_pull_request_messages;
    uint64_t globalq_data_pull_request_total_bytes;
    uint64_t globalq_data_pull_reply_messages;
    uint64_t globalq_data_pull_reply_total_bytes;

    // global queue elements transfered
    TotalStatistic globalq_data_pull_request_num_elements;
    TotalStatistic globalq_data_pull_reply_num_elements;

#ifdef VTRACE
    unsigned steal_queue_grp_vt;

    unsigned share_request_ev_vt;
    unsigned share_reply_ev_vt;

    unsigned globalq_data_pull_request_ev_vt;
    unsigned globalq_data_pull_reply_ev_vt;
    unsigned globalq_data_pull_reply_num_elements_ev_vt;
#endif

  public:
    StealStatistics();
    void reset();
    void record_steal_reply( size_t msg_bytes ); 
    void record_steal_request( size_t msg_bytes ); 
    void record_workshare_request( size_t msg_bytes );
    void record_workshare_reply( size_t msg_bytes, bool isAccepted, int num_received, int num_denying, int num_sending );
    void record_workshare_reply_nack( size_t msg_bytes );
    void record_globalq_data_pull_reply( size_t msg_bytes, uint64_t amount );
    void record_globalq_data_pull_request( size_t msg_bytes, uint64_t amount );
    void dump( std::ostream& o = std::cout, const char * terminator = "" );
    void merge( const StealStatistics * other );
    void profiling_sample();
};

extern StealStatistics steal_queue_stats;

/// Bounded queue that knows how to share elements
/// with other queues by work stealing.
///
/// @tparam T type of elements
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

    // work stealing 
    void steal_reply( uint64_t amt, uint64_t total, T * stolen_work, size_t stolen_size_bytes );
    void steal_request( int k, Node from );

    // work sharing
    void workShareRequest( uint64_t remoteSize, Node from, T * data, int num );
    void workShareReplyFewer( int amountDenied );
    void workShareReplyGreater( int amountGiven, T * data );

    // global queue
    void pull_global_data_request( pull_global_data_args<T> * args );
    void pull_global_data_reply( GlobalAddress< Signaler > * signal, T * received_elements, size_t elements_size );

    /* The number of elements that have been released
     * below <bottom> but not yet copied out. Reclaiming
     * array space is only allowed if this is zero 
     */
    uint64_t numPendingElements;

    /* The stack is a non-circular array. This routine
     * reclaims empty space without copying elements
     * if it is safe.
     */
    void reclaimSpace();

    // work stealing dispatch
    static void workStealReply_am( workStealReply_args * args,  size_t size, void * payload, size_t payload_size );
    static void workStealRequest_am( workStealRequest_args * args, size_t size, void * payload, size_t payload_size );

    // work sharing dispatch
    static void workShareRequest_am ( workShareRequest_args * args, size_t args_size, void * payload, size_t payload_size );
    static void workShareReplyFewer_am ( workShareReply_args * args, size_t args_size, void * payload, size_t payload_size );
    static void workShareReplyGreater_am ( workShareReply_args * args, size_t args_size, void * payload, size_t payload_size );

    // global queue dispatch
    static void pull_global_data_request_g_am( pull_global_data_args<T> * args, size_t args_size, void * payload, size_t payload_size );
    static void pull_global_data_reply_g_am( GlobalAddress< Signaler > * signal, size_t arg_size, T * payload, size_t payload_size );

    /// Output stream of queue state
    std::ostream& dump ( std::ostream& o ) const {
      std::stringstream ss;
      for ( uint64_t i = top; i>bottom; i-- ) {
        ss << stack[i-1];
        ss << ",\n";
      }
      return o << "StealQueue[depth=" << depth()
        << "; indices(top= " << top 
        << " bottom=" << bottom << ")"
        << "; stackSize=" << stackSize 
        << "; contents=\n" << ss.str() << "]";
    }

  public:
    static StealQueue<T> steal_queue;

    void init( uint64_t numEle ) {
      stackSize = numEle;

      uint64_t nbytes = numEle * sizeof(T);

      // allocate stack in shared addr space with affinity to calling thread
      // and record local addr for efficient access in sequel
      stack_g = static_cast<T*>( malloc( nbytes ) );
      stack = stack_g;

      CHECK( stack!= NULL ) << "Request for " << nbytes << " bytes for stealStack failed";

      mkEmpty();

    }

    /// Constructor allocates uninitialized queue
    StealQueue( ) 
      : stackSize( -1 )
        , maxStackDepth( 0 )
        , nNodes( 0 ), maxTreeDepth( 0 ), nVisited( 0 ), nLeaves( 0 )
        , nAcquire( 0 ), nRelease( 0 ), nStealPackets( 0 ), nFail( 0 )
        , wakeups( 0 ), falseWakeups( 0 ), nNodes_last( 0 ) 
        , numPendingElements( 0 )
  { }

    void mkEmpty(); 
    void push( T c); 
    T peek( ); 
    void pop( ); 
    uint64_t topPosn( ) const;
    uint64_t depth( ) const; 
    void release( int k ); 
    int acquire( int k ); 

    /// Get number of elements that have been
    /// pushed into this queue
    uint64_t get_nNodes( ) {
      return nNodes;
    }

    /// register local address of remote steal queues
    static void registerAddress( StealQueue<T> * addr );

    template< typename U >
      friend std::ostream& operator<<( std::ostream& o, const StealQueue<U>& sq );

    static const int bufsize = 110; // TODO: I know 32B*110 < aggreg bufsize-header size
    // but do this precisely

    // work stealing API
    int steal_locally( Node victim, int chunkSize ); 

    // work sharing API
    int64_t workShare( Node target, uint64_t amount );

    // global queue API
    uint64_t pull_global();
    bool push_global( uint64_t amount );

};

static int maxint(int x, int y) { return (x>y)?x:y; }

/// Push onto top of local stack
template <typename T>
inline void StealQueue<T>::push( T c ) {
  CHECK( top < stackSize ) << "push: overflow (top:" << top << " stackSize:" << stackSize << ")";

  VLOG(5) << "stack[" << top << "] <-- push";
  stack[top] = c; 
  top++;
  nNodes++;
  maxStackDepth = maxint(top, maxStackDepth);
  //s->maxTreeDepth = maxint(s->maxTreeDepth, c->height); //XXX dont want to deref c here (expensive for just a bookkeeping operation

  DVLOG(5) << "after push:" << *this;
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

  DVLOG(5) << "after pop:" << *this;
}

/// number of elements in the queue
template <typename T>
inline uint64_t StealQueue<T>::depth() const {
  return (top - bottom);
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

//#include "../Grappa.hpp" 
#include <Communicator.hpp>
#include <tasks/TaskingScheduler.hpp>

extern TaskingScheduler global_scheduler;
// void Grappa_suspend();
// void Grappa_wake( Thread * );
// Node Grappa_mynode();

/// Arguments for a work steal request from thief
struct workStealRequest_args {
  int k;
  Node from;
};

/// Arguments for a work steal reply from victim
struct workStealReply_args {
  int stealAmt;
  int total;
};

static int64_t local_steal_amount;
static uint64_t received_tasks;
static Thread * steal_waiter = NULL;

static int64_t local_push_retVal = -1;
static int64_t local_push_amount = 0;
static bool local_push_replyfewer;
static uint64_t local_push_old_bottom;
static Thread * push_waiter = NULL;
static bool pendingWorkShare = false;

static bool pendingGlobalPush = false;

/// Reply to steal operation that takes place on the thief's Node
/// Copies the received elements into the local queue
template <typename T>
void StealQueue<T>::steal_reply( uint64_t amt, uint64_t total, T * stolen_work, size_t stolen_size_bytes ) {
  if (amt > 0) {
    GRAPPA_EVENT(steal_packet_ev, "Steal packet", 1, scheduler, amt);

#ifdef VTRACE
    //VT_COUNT_UNSIGNED_VAL( thiefStack->steal_success_ev_vt, k );
#endif

    reclaimSpace();

    CHECK( top + amt < stackSize ) << "steal reply: overflow (top:" << top << " stackSize:" << stackSize << " amt:" << amt << ")";
    memcpy(&stack[top], stolen_work, stolen_size_bytes);

    received_tasks += amt;
    VLOG(5) << "Steal packet returns with amt=" << amt << ", received=" << received_tasks << " / total=" << total;
    top += amt;
    nStealPackets++;

    /// The steal requestor stays asleep until all received, but other threads can take advantage
    /// of the tasks that have been copied in
    if ( received_tasks == total ) { 
      GRAPPA_EVENT(steal_success_ev, "Steal success", 1, scheduler, total);
      VLOG(5) << "Last packet; will wake steal_waiter=" << steal_waiter;
      local_steal_amount = total;
      if ( steal_waiter != NULL ) {
        //Grappa_wake( steal_waiter );
        global_scheduler.thread_wake( steal_waiter );
        steal_waiter = NULL;
      }
    }

  } else {
    local_steal_amount = 0;
    nFail++;

    if ( steal_waiter != NULL ) {
      //Grappa_wake( steal_waiter );
      global_scheduler.thread_wake( steal_waiter );
      steal_waiter = NULL;
    }
  }
}

/// Steal reply Grappa active message
template <typename T>
void StealQueue<T>::workStealReply_am( workStealReply_args * args,  size_t size, void * payload, size_t payload_size ) {
  CHECK ( local_steal_amount == -1 ) << "local_steal_amount=" << local_steal_amount << " when steal reply arrives";
  CHECK( args->stealAmt * sizeof(T) == payload_size ) << "steal amount in bytes != payload size";

  T * stolen_work = static_cast<T*>( payload );

  steal_queue.steal_reply( args->stealAmt, args->total, stolen_work, payload_size );
}

/// Steal request Grappa active message
template <typename T>
void StealQueue<T>::steal_request( int k, Node from ) {
  int victimBottom = this->bottom;
  int victimTop = this->top;

  const int victimHalfWorkAvail = (victimTop - victimBottom) / 2;
  const int stealAmt = MIN_INT( victimHalfWorkAvail, k );
  bool ok = stealAmt > 0;

  VLOG(4) << "Victim of thief=" << from << " victimHalfWorkAvail=" << victimHalfWorkAvail;
  if (ok) {

    /* reserve a chunk */
    this->bottom = victimBottom + stealAmt;


    GRAPPA_EVENT(steal_victim_ev, "Steal victim", 1, scheduler, stealAmt);
#ifdef VTRACE
    //VT_COUNT_UNSIGNED_VAL( steal_victim_ev_vt, k );
#endif

    T* victimStackBase = this->stack;
    T* victimStealStart = victimStackBase + victimBottom;

    int offset = 0;
    for ( int remain = stealAmt; remain > 0; ) {
      int transfer_amt = (remain < bufsize) ? remain : bufsize;
      VLOG(5) << "sending steal packet of transfer_amt=" << transfer_amt << " remain=" << remain << " / stealAmt=" << stealAmt;
      remain -= transfer_amt;
      workStealReply_args reply_args = { transfer_amt, stealAmt };

      Grappa_call_on( from, &StealQueue<T>::workStealReply_am, 
          &reply_args, sizeof(workStealReply_args), 
          victimStealStart + offset, transfer_amt*sizeof( T ));
      size_t msg_size = sizeof(reply_args) + transfer_amt * sizeof(T) + Grappa_sizeof_header();
      steal_queue_stats.record_steal_reply( msg_size );

      offset += transfer_amt;
    }

#if DEBUG
    // 0 out the stolen stuff (to detect errors)
    memset( victimStealStart, 0, stealAmt*sizeof( T ) );
#endif

  } else {
    workStealReply_args reply_args = { 0, 0 };
    Grappa_call_on( from, &StealQueue<T>::workStealReply_am, &reply_args );
    size_t msg_size = sizeof(reply_args) + Grappa_sizeof_header();
    steal_queue_stats.record_steal_reply( msg_size );
  }
}

template <typename T>
void StealQueue<T>::workStealRequest_am(workStealRequest_args * args, size_t size, void * payload, size_t payload_size) {
  int k = args->k;
  Node from = args->from;

  steal_queue.steal_request( k, from );
}

#include "Thread.hpp"

/// Steal elements from the StealQueue<T> located at the victim Node.
/// @tparam T type of the queue elements
/// @param victim target Node to steal from
/// @param op max steal amount
template <typename T>
int StealQueue<T>::steal_locally( Node victim, int op ) {

  // initialize stealing state
  local_steal_amount = -1;
  received_tasks = 0;

  workStealRequest_args req_args = { op, global_communicator.mynode() };
  Grappa_call_on( victim, &StealQueue<T>::workStealRequest_am, &req_args );
  size_t msg_size = sizeof(req_args) + Grappa_sizeof_header();
  steal_queue_stats.record_steal_request( msg_size );

  GRAPPA_PROFILE_CREATE( stealprof, "steal_locally", "(suspended)", GRAPPA_SUSPEND_GROUP );

  reclaimSpace(); 

  // steal is blocking
  // TODO: use suspend-wake mechanism
  while ( local_steal_amount == -1 ) {
    steal_waiter = global_scheduler.get_current_thread();

    GRAPPA_PROFILE_THREAD_START( stealprof, global_scheduler.get_current_thread() );

    global_scheduler.thread_suspend();
    //Grappa_suspend();

    GRAPPA_PROFILE_THREAD_STOP( stealprof, global_scheduler.get_current_thread() );
  }

  return local_steal_amount;
}
/////////////////////////////////////////////////////////

struct workShareRequest_args {
  uint64_t queueSize;
  int amountPushed;
  Node from;
};

/// returns change in number of elements
template <typename T>
int64_t StealQueue<T>::workShare( Node target, uint64_t amount ) {
  CHECK( !pendingWorkShare ) << "Implementation allows only one pending workshare per node";
  CHECK( global_communicator.mynode() != target ) << "cannot workshare with self target: " << target;
  CHECK( amount <= bufsize ) << "Only support single-packet transfers";

  uint64_t mySize = depth();

  reclaimSpace();

  // initialize sharing state
  local_push_retVal = -1;

  local_push_amount = amount;

  uint64_t origBottom = bottom;
  uint64_t origTop = top;

  // reserve a chunk
  bottom = bottom + amount;

  T * xfer_stackBase = stack;
  T * xfer_start = xfer_stackBase + origBottom;

  pendingWorkShare = true;
  local_push_old_bottom = origBottom;

  DVLOG(5) << "Initiating work share: target=" << target << ", mySize=" << mySize << ", amount=" << amount << ", new bottom=" << bottom;

  workShareRequest_args args = { mySize, amount, global_communicator.mynode() };
  Grappa_call_on( target, StealQueue<T>::workShareRequest_am, &args, sizeof(args), xfer_start, amount * sizeof(T) );
  size_t msg_size = sizeof(args) + amount * sizeof(T) + Grappa_sizeof_header();
  steal_queue_stats.record_workshare_request( msg_size );

  if ( local_push_retVal < 0 ) {
    push_waiter = global_scheduler.get_current_thread();
    global_scheduler.thread_suspend();
    CHECK( local_push_retVal >= 0 );
  }

  DVLOG(5) << "Initiator wakes from work share: target=" << target << ", updated bottom=" << bottom;

  pendingWorkShare = false;

  if ( local_push_replyfewer ) {
    DVLOG(5) << "  woken by: target " << target << ", had fewer and denied " << local_push_retVal;

    // amount pushed = total amount - amount denied
    return -(amount - local_push_retVal);
  } else {
    DVLOG(5) << "  woken by: target " << target << ", had greater so denied all and sent " << local_push_retVal;

    // amount received
    return local_push_retVal;
  }
}

struct workShareReply_args {
  int amount;
};

template <typename T>
void StealQueue<T>::reclaimSpace() {
  // reclaim space if the queue is empty
  // and there is no pending transfer below 'bottom' (workshare or pending global q pull)
  if ( depth() == 0 && !pendingWorkShare && numPendingElements == 0 ) {
    mkEmpty();
  }
}

template <typename T>
void StealQueue<T>::workShareReplyFewer( int amountDenied ) {
  // restore denied work
  CHECK( bottom >= amountDenied ) << "bottom = " << bottom 
    << " amountDenied = " << amountDenied;

  // invariant that bottom does not change
  CHECK( bottom == local_push_old_bottom+local_push_amount );

  uint64_t prev_bottom = bottom;
  bottom -= amountDenied;

#if DEBUG
  T * xfer_start = stack + local_push_old_bottom;

  // 0 out the transfered stuff (to detect errors)
  memset(xfer_start, 0, (local_push_amount-amountDenied)*sizeof( T ) );
#endif

  DVLOG(5) << "replyFewer: " << amountDenied << " denied; moving bottom " << prev_bottom << " -> " << bottom
    << " " << *this;

  local_push_replyfewer = true;
  local_push_retVal = amountDenied;
  if ( push_waiter != NULL ) {
    global_scheduler.thread_wake( push_waiter );
    push_waiter = NULL;
  }

}

template <typename T>
void StealQueue<T>::workShareReplyGreater( int amountGiven, T * data ) {
  // restore all pushed work
  CHECK( bottom >= local_push_amount );

  // invariant that bottom does not change
  CHECK( bottom == local_push_old_bottom+local_push_amount );

  uint64_t prev_bottom = bottom;
  bottom -= local_push_amount;


  // copy received work onto stack
  CHECK( top + amountGiven < stackSize ) << "steal reply: overflow (top:" << top << " stackSize:" << stackSize << " amt:" << amountGiven << ")";
  memcpy(&stack[top], data, amountGiven * sizeof(T));

  top += amountGiven;

  DVLOG(5) << "replyGreater: " << amountGiven << " given; moving bottom " << prev_bottom << " -> " << bottom
    << " " << *this;

  local_push_replyfewer = false;
  local_push_retVal = amountGiven;
  if ( push_waiter != NULL ) {
    global_scheduler.thread_wake( push_waiter );
    push_waiter = NULL;
  }
}

template <typename T>
void StealQueue<T>::workShareReplyFewer_am ( workShareReply_args * args, size_t args_size, void * payload, size_t payload_size ) {
  steal_queue.workShareReplyFewer( args->amount );
}

template <typename T>
void StealQueue<T>::workShareReplyGreater_am ( workShareReply_args * args, size_t args_size, void * payload, size_t payload_size ) {
  CHECK( payload_size == args->amount * sizeof(T) );

  steal_queue.workShareReplyGreater( args->amount, static_cast<T*>( payload ) );
}

template <typename T>
void StealQueue<T>::workShareRequest_am ( workShareRequest_args * args, size_t args_size, void * payload, size_t payload_size ) {
  CHECK( payload_size == args->amountPushed * sizeof(T) );

  steal_queue.workShareRequest( args->queueSize, args->from, static_cast<T*>( payload ), args->amountPushed );
}

DECLARE_int32( chunk_size );
template <typename T>
void StealQueue<T>::workShareRequest( uint64_t remoteSize, Node from, T * data, int num ) {
  uint64_t mySize = depth();

  reclaimSpace();

  int64_t diff = mySize - remoteSize;
  if ( diff > 0 ) {
    // we have more elements, so ignore the incoming data and send some

    // cannot violate that bottom is constant during pendingWorkShare=true
    if ( pendingWorkShare ) {
      // reply that all work is denied, none sent
      workShareReply_args reply_args = { num };
      Grappa_call_on ( from, &StealQueue<T>::workShareReplyFewer_am, &reply_args );
      size_t msg_size = sizeof(reply_args) + Grappa_sizeof_header();
      steal_queue_stats.record_workshare_reply_nack( msg_size );
      return;
    }

    // no pendingWorkShare, so proceed
    uint64_t balanceAmount = ((mySize+remoteSize)/2) - remoteSize;
    int amountToSend = MIN_INT( balanceAmount, FLAGS_chunk_size ); // restrict to chunk size; TODO: don't use flag
    CHECK( amountToSend >= 0 ) << "amountToSend = " << amountToSend;

    uint64_t origBottom = bottom;
    uint64_t origTop = top;

    // reserve a chunk
    bottom = bottom + amountToSend;

    T * xfer_stackBase = stack;
    T * xfer_start = xfer_stackBase + origBottom;

    DVLOG(5) << "from=" << from << ", size=" << mySize << " vs " << remoteSize << ", received " << num << ", sending " << amountToSend;

    // reply with number of elements being sent
    workShareReply_args reply_args = { amountToSend };
    Grappa_call_on( from, &StealQueue<T>::workShareReplyGreater_am, &reply_args, sizeof(reply_args), xfer_start, amountToSend * sizeof(T) );
    size_t msg_size = sizeof(reply_args) + amountToSend * sizeof(T) + Grappa_sizeof_header();
    steal_queue_stats.record_workshare_reply( msg_size, false, num, num, amountToSend );

#if DEBUG
    // 0 out the transfered stuff (to detect errors)
    memset(xfer_start, 0, amountToSend*sizeof( T ) );
#endif
  } else {
    // we have fewer elements, so take it and reply that we had fewer elements in the queue

    uint64_t balanceAmount = ((mySize+remoteSize)/2) - mySize;
    int amountToTake = MIN_INT( balanceAmount, num );
    CHECK( amountToTake >= 0 ) << "amountToTake = " << amountToTake;

    DVLOG(5) << "from=" << from << ", size=" << mySize << " vs " << remoteSize << ", received " << num << ", taking " << amountToTake;

    // TODO consider below bottom
    CHECK( top + amountToTake < stackSize );
    memcpy(&stack[top], data, amountToTake * sizeof(T) );
    top += amountToTake;

    // reply with number of elements denied
    int denied = num - amountToTake;
    workShareReply_args reply_args = { denied };
    Grappa_call_on ( from, &StealQueue<T>::workShareReplyFewer_am, &reply_args );
    size_t msg_size = sizeof(reply_args) + Grappa_sizeof_header();
    steal_queue_stats.record_workshare_reply( msg_size, true, num, denied, 0 );
  }
}


///////////////////////////
// Global queue interaction
///////////////////////////

class Signaler;

template <typename T>
uint64_t StealQueue<T>::pull_global() {
  ChunkInfo<T> data_ptr;
  global_queue_pull<T>( &data_ptr );

  Signaler signal;
  pull_global_data_args<T> args;
  args.signal = make_global( &signal );
  args.chunk = data_ptr;
  Grappa_call_on( data_ptr.base.node(), pull_global_data_request_g_am, &args );
  size_t msg_size = sizeof(args) + Grappa_sizeof_header();
  steal_queue_stats.record_globalq_data_pull_request( msg_size, data_ptr.amount );
  signal.wait();

  return data_ptr.amount;
}

template <typename T>
void StealQueue<T>::pull_global_data_request_g_am( pull_global_data_args<T> * args, size_t args_size, void * payload, size_t payload_size ) {
  steal_queue.pull_global_data_request( args );
}

template <typename T>
void StealQueue<T>::pull_global_data_request( pull_global_data_args<T> * args ) {
  CHECK( numPendingElements >= args->chunk.amount ) << "trying to take more than released number of elements";

  T * chunk_base = args->chunk.base.pointer();
  CHECK( chunk_base >= stack && chunk_base < stack+stackSize ) << "chunk base pointer falls outside of the stack range";

  CHECK( chunk_base + args->chunk.amount <= stack+bottom ) << "chunk overlaps the local part of the stack";

  Grappa_call_on_x( args->signal.node(), pull_global_data_reply_g_am, &(args->signal), sizeof(args->signal), chunk_base, args->chunk.amount * sizeof(T) );
  size_t msg_size = sizeof(args->signal) + args->chunk.amount * sizeof(T) + Grappa_sizeof_header();
  steal_queue_stats.record_globalq_data_pull_reply( msg_size, args->chunk.amount );

  numPendingElements -= args->chunk.amount;

#if DEBUG
  // 0 out the transfered elements (to detect errors)
  memset( chunk_base, 0, args->chunk.amount*sizeof(T) );
#endif

  // in case all pending elements are now gone, try to reclaim space
  reclaimSpace();
}

template <typename T>
void StealQueue<T>::pull_global_data_reply_g_am( GlobalAddress< Signaler > * signal, size_t arg_size, T * payload, size_t payload_size ) {
  steal_queue.pull_global_data_reply( signal, payload, payload_size );
}

template <typename T>
void StealQueue<T>::pull_global_data_reply( GlobalAddress< Signaler > * signal, T * received_elements, size_t elements_size ) {
  uint64_t num_elements = elements_size / sizeof(T);

  CHECK( top + num_elements < stackSize ) << "pull_global_data reply: overflow (top:" << top << " stackSize:" << stackSize << " amt:" << num_elements << ")";

  // TODO bottom better
  memcpy( &stack[top], received_elements, elements_size );
  top += num_elements;

  // wake the thread that initiated the pull
  signal->pointer()->signal();
}

template <typename T>
bool StealQueue<T>::push_global( uint64_t amount ) {
  CHECK( !pendingGlobalPush ) << "multiple outstanding pushes not allowed"; // due to reclaiming unaccepted elements
  pendingGlobalPush = true;

  // check the amount is legal
  CHECK( amount <= depth() ) << "trying to release more elements than are in the queue";
  CHECK( amount <= bufsize ) << "Only support single-packet transfers";

  // release elements 
  uint64_t orig_bottom = bottom;
  bottom += amount;
  numPendingElements += amount;

  // push chunk information to the global queue.
  // this is a blocking/yielding operation
  bool accepted = global_queue_push<T>( make_global( &stack[orig_bottom] ), amount );

  if ( !accepted ) {
    // reclaim unaacepted elements
    bottom = orig_bottom;
    numPendingElements -= amount;
  }

  pendingGlobalPush = false;

  return accepted;
}

// allocation of steal_queue instance
template <typename T>
StealQueue<T> StealQueue<T>::steal_queue;

/// Output stream for state of the StealQueue
template <typename T>
std::ostream& operator<<( std::ostream& o, const StealQueue<T>& sq ) {
  return sq.dump( o );
}

#endif // STEALQUEUE_HPP
