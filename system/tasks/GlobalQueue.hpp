#ifndef __GLOBAL_QUEUE_HPP__
#define __GLOBAL_QUEUE_HPP__

//TODO clean these up
#include "../Addressing.hpp" 
#include "../GlobalAllocator.hpp"
#include "../Delegate.hpp"
#include "../Cache.hpp"
#include "../ConditionVariables.hpp"
#include "../Descriptor.hpp"

#define CAPACITY_PER_NODE (1L<<19)
#define HOME_NODE 0

class GlobalQueueStatistics {
  private:
    // network usage
    uint64_t globalq_pull_reserve_request_messages;
    uint64_t globalq_pull_reserve_request_total_bytes;
    
    uint64_t globalq_pull_reserve_reply_messages;
    uint64_t globalq_pull_reserve_reply_total_bytes;
    
    uint64_t globalq_push_reserve_request_messages;
    uint64_t globalq_push_reserve_request_total_bytes;
    
    uint64_t globalq_push_entry_request_messages;
    uint64_t globalq_push_entry_request_total_bytes;
    
    uint64_t globalq_push_reserve_reply_messages;
    uint64_t globalq_push_reserve_reply_total_bytes;

    // pull
    uint64_t globalq_pull_request_granted;
    uint64_t globalq_pull_request_denied;
    uint64_t globalq_pull_entry_request_messages;
    uint64_t globalq_pull_entry_request_total_bytes;
    uint64_t globalq_pull_entry_reply_messages;
    uint64_t globalq_pull_entry_reply_total_bytes;
    
    // push
    uint64_t globalq_push_request_accepted;
    uint64_t globalq_push_request_rejected;
    uint64_t globalq_push_entry_hadConsumer;
    uint64_t globalq_push_entry_noConsumer;
  
  
  public:
    GlobalQueueStatistics();
    void reset();
    void dump(); 
    void merge( const GlobalQueueStatistics * other );
    void profiling_sample();

    void record_push_reserve_request( size_t msg_bytes, bool accepted );
    void record_push_entry_request( size_t msg_bytes, bool had_consumer );
    void record_pull_reserve_request( size_t msg_bytes, bool granted );
    void record_pull_entry_request( size_t msg_bytes );
    void record_push_reserve_reply( size_t msg_bytes );
    void record_pull_reserve_reply( size_t msg_bytes );
    void record_pull_entry_reply( size_t msg_bytes );
};

extern GlobalQueueStatistics global_queue_stats;


template <typename T>
struct ChunkInfo {
  GlobalAddress<T> base;
  uint64_t amount;
};

template <typename T>
struct QueueEntry {
  ChunkInfo<T> chunk;
  GlobalAddress<Signaler> sleeper;
  bool valid;
};

template <typename T>
struct push_entry_args {
  ChunkInfo<T> chunk;
  GlobalAddress< QueueEntry<T> > target;
};

template <typename T>
struct pull_entry_args {
  GlobalAddress< QueueEntry<T> > target;
  GlobalAddress< Descriptor< ChunkInfo<T> > > descriptor;
};

template <typename T>
class GlobalQueue {

  private:
    uint64_t head;
    uint64_t tail;
    GlobalAddress< QueueEntry<T> > queueBase;
    uint64_t capacity;

    bool initialized;

    static bool isMaster() {
      return SoftXMT_mynode() == HOME_NODE;
    }

    
    GlobalAddress< QueueEntry<T> > push_reserve ( bool ignore );
    GlobalAddress< QueueEntry<T> > pull_reserve ( bool ignore );
    void pull_entry_sendreply( GlobalAddress< Descriptor< ChunkInfo<T> > > desc, QueueEntry<T> * e );
    bool push_entry ( push_entry_args<T> args );
    void pull_entry_request( pull_entry_args<T> * args );

    static GlobalAddress< QueueEntry<T> > push_reserve_g ( bool ignore );
    static GlobalAddress< QueueEntry<T> > pull_reserve_g ( bool ignore );
    static bool push_entry_g ( push_entry_args<T> args );
    static void pull_entry_request_g_am( pull_entry_args<T> * args, size_t args_size, void * payload, size_t payload_size );
    
  public:
    static GlobalQueue<T> global_queue;

    void init() {
      if ( isMaster() ) {
        capacity = (CAPACITY_PER_NODE) * SoftXMT_nodes();
        DVLOG(5) << "GlobalQueue capacity: " << capacity;
        queueBase = SoftXMT_typed_malloc< QueueEntry<T> > ( capacity );
          // TODO could give option just to malloc here, but would be
          // bad for HOME_NODE's memory usage unless chunksize is very large
      }
      initialized = true;
    }

    GlobalQueue() 
        : head ( 0 )
        , tail ( 0 )
        , capacity ( 0 ) 
        , initialized( false ) {}

    
    bool pull( ChunkInfo<T> * result );
    bool push( GlobalAddress<T> chunk_base, uint64_t chunk_amount );

};

/**
 *  Push a chunk of data to the global queue.
 *  The data remains in-place until consumed.
 *
 *  \param chunk_base global pointer to the pushed chunk
 *  \param chunk_amount number of elements in pushed chunk
 *
 *  \return true if push succeeds
 */
template <typename T>
bool GlobalQueue<T>::push( GlobalAddress<T> chunk_base, uint64_t chunk_amount ) {
  CHECK( initialized );
  DVLOG(5) << "push() base:" << chunk_base << " amount:" << chunk_amount;

  GlobalAddress< QueueEntry<T> > loc = SoftXMT_delegate_func< bool, GlobalAddress< QueueEntry<T> >, GlobalQueue<T>::push_reserve_g > ( false, HOME_NODE );
  size_t msg_bytes = SoftXMT_sizeof_delegate_func_request< bool, GlobalAddress< QueueEntry<T> > >( );
  
  DVLOG(5) << "push() reserve done -- loc:" << loc;
  
  if ( loc.pointer() == NULL ) {
    global_queue_stats.record_push_reserve_request( msg_bytes, false );
    // no space in global queue; push failed
    return false;
  }
    
  global_queue_stats.record_push_reserve_request( msg_bytes, true );

  // push the queue entry that points to my chunk 
  ChunkInfo<T> c;
  c.base = chunk_base;
  c.amount = chunk_amount;
  push_entry_args<T> entry_args;
  entry_args.target = loc;
  entry_args.chunk = c;
  DVLOG(5) << "push() sending entry to " << loc;
  bool had_sleeper = SoftXMT_delegate_func< push_entry_args<T>, bool, GlobalQueue<T>::push_entry_g > ( entry_args, loc.node() ); 
  size_t entry_msg_bytes = SoftXMT_sizeof_delegate_func_request< push_entry_args<T>, bool >( );
  global_queue_stats.record_push_entry_request( entry_msg_bytes, had_sleeper );

  return true;
}

/**
 * Get info to pull a chunk of data from the global queue.
 * If it returns true, then we've been granted a chunk that
 * can be pulled.
 *
 * \return true if pull succeeds.
 */
template <typename T>
bool GlobalQueue<T>::pull( ChunkInfo<T> * result ) {
  CHECK( initialized );
  DVLOG(5) << "pull";
  
  GlobalAddress< QueueEntry<T> > loc = SoftXMT_delegate_func< bool,
                                                GlobalAddress< QueueEntry<T> >,
                                                GlobalQueue<T>::pull_reserve_g >
                                            ( false, HOME_NODE );
  size_t resv_msg_bytes = SoftXMT_sizeof_delegate_func_request< bool, GlobalAddress< QueueEntry<T> > >( );

  if ( loc.pointer() == NULL ) {
    global_queue_stats.record_pull_reserve_request( resv_msg_bytes, false );
    return false; // no space in global queue; push failed
  }
 
  global_queue_stats.record_pull_reserve_request( resv_msg_bytes, true );

  // get the element of the queue, which will point to data
  Descriptor< ChunkInfo<T> > desc( result );

  pull_entry_args<T> entry_args;
  entry_args.target = loc;
  entry_args.descriptor = make_global( &desc );
  SoftXMT_call_on( loc.node(), pull_entry_request_g_am, &entry_args );
  size_t entry_msg_bytes = SoftXMT_sizeof_message( &entry_args );
  global_queue_stats.record_pull_entry_request( entry_msg_bytes );

  desc.wait();

  return true;
}

template <typename T>
GlobalAddress< QueueEntry<T> > GlobalQueue<T>::push_reserve ( bool ignore ) {
  CHECK( isMaster() );

  global_queue_stats.record_push_reserve_reply( SoftXMT_sizeof_delegate_func_reply< bool, GlobalAddress< QueueEntry<T> > >() );
  
  DVLOG(5) << "push_reserve";

  CHECK( capacity > 0 );
  if ( (tail % capacity == head % capacity) && (tail != head) ) {
    return make_global( static_cast< QueueEntry<T> * >( NULL ) ); // no room
  } else {
    GlobalAddress< QueueEntry<T> > assigned = queueBase + (tail % capacity); 
    tail++;
    return assigned;
  }
}

template <typename T>
GlobalAddress< QueueEntry<T> > GlobalQueue<T>::pull_reserve ( bool ignore ) {
  CHECK( isMaster() );
  DVLOG(5) << "pull_reserve";

  global_queue_stats.record_pull_reserve_reply( SoftXMT_sizeof_delegate_func_reply< bool, GlobalAddress< QueueEntry<T> > > () );
  
  if ( tail == head ) {
    return make_global( static_cast<QueueEntry<T> * >( NULL ) ); // empty
  } else {
    GlobalAddress< QueueEntry<T> > granted = queueBase + (head % capacity);
    head++;
    return granted;
  }
}

template <typename T>
void GlobalQueue<T>::pull_entry_sendreply( GlobalAddress< Descriptor< ChunkInfo<T> > > desc, QueueEntry<T> * e ) {
  // consume entry
  e->valid = false;

  // send data to puller
  descriptor_reply_one( desc, &(e->chunk) );
  size_t msg_bytes = SoftXMT_sizeof_descriptor_reply_one( desc, &(e->chunk) );
  global_queue_stats.record_pull_entry_reply( msg_bytes );
}

template <typename T>
bool GlobalQueue<T>::push_entry ( push_entry_args<T> args ) {
  QueueEntry<T> * e = args.target.pointer();
    
  e->chunk = args.chunk;
  e->valid = true;
  
  // if a consumer is waiting then send a wake message
  if ( e->sleeper.pointer() != NULL ) {
    DVLOG(5) << "push_entry: was sleeping consumer " << e->sleeper;

    pull_entry_sendreply( e->sleeper, e );
    e->sleeper = make_global( static_cast<Descriptor< ChunkInfo<T> > * >( NULL ) );
    return true;
  } else {
    DVLOG(5) << "push_entry: no consumer";
    return false;
  }
} 

template <typename T>
void GlobalQueue<T>::pull_entry_request( pull_entry_args<T> * args ) {
  // for now just a valid bit: empty or full, no nack for retry

  QueueEntry<T> * e = args->target.pointer();  
  if ( !e->valid ) {
    // leave a note to wake the client.
    // send no reply yet; the client will block
    e->sleeper = args->descriptor; 
  } else {
    pull_entry_sendreply( args->descriptor, e );
  }
}
    

// routines for calling the global GlobalQueue on each Node
template <typename T>
GlobalAddress< QueueEntry<T> > GlobalQueue<T>::push_reserve_g ( bool ignore ) {
  return global_queue.push_reserve( ignore );
}

template <typename T>
GlobalAddress< QueueEntry<T> > GlobalQueue<T>::pull_reserve_g ( bool ignore ) {
  return global_queue.pull_reserve( ignore );
}

template <typename T>
bool GlobalQueue<T>::push_entry_g ( push_entry_args<T> args ) {
  return global_queue.push_entry( args );
}

template <typename T>
void GlobalQueue<T>::pull_entry_request_g_am( pull_entry_args<T> * args, size_t args_size, void * payload, size_t payload_size ) {
  global_queue.pull_entry_request( args );
} 

// allocation of global_queue instance
template <typename T>
GlobalQueue<T> GlobalQueue<T>::global_queue;


// global object GlobalQueue<T>::global_queue convenience methods
template <typename T>
bool global_queue_pull( ChunkInfo<T> * result ) {
  GlobalQueue<T>::global_queue.pull( result );
}
template <typename T>
bool global_queue_push( GlobalAddress<T> chunk_base, uint64_t chunk_amount ) {
  GlobalQueue<T>::global_queue.push( chunk_base, chunk_amount );
}




#endif //__GLOBAL_QUEUE_HPP__
