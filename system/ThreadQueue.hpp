#pragma once

#include "Worker.hpp"
#include <limits>
#include <glog/logging.h>


/// A queue of threads
class ThreadQueue {
    private:
        Worker * head;
        Worker * tail;
        uint64_t len;
        
        std::ostream& dump( std::ostream& o ) const {
            return o << "[length:" << len
                     << "; head:" << (void*)head
                     << "; tail:" << (void*)tail << "]";
        }

    public:
        ThreadQueue ( ) 
            : head ( NULL )
            , tail ( NULL )
            , len ( 0 ) { }

        void enqueue(Worker * t);
        Worker * dequeue();
        void prefetch();
        uint64_t length() { 
            return len;
        }

        bool empty() {
            return (head==NULL);
        }
        
        friend std::ostream& operator<< ( std::ostream& o, const ThreadQueue& tq );
};


template< typename T >
T _max(const T& a, const T& b) {
  return (a>b) ? a : b;
}
template< typename T >
T _min(const T& a, const T& b) {
  return (a<b) ? a : b;
}


class PrefetchingThreadQueue {
  private:
    ThreadQueue * queues;
    uint64_t eq_index;
    uint64_t dq_index;
    uint64_t num_queues;
    uint64_t len;

    void check_invariants() {
      uint64_t max=std::numeric_limits<uint64_t>::min();
      uint64_t min=std::numeric_limits<uint64_t>::max();
      uint64_t sum=0;
      for ( uint64_t i=0; i<num_queues; i++ ) {
        max = _max<uint64_t>( max, queues[i].length() );
        min = _min<uint64_t>( min, queues[i].length() );
        sum += queues[i].length();
      }
      CHECK( max - min >= 0 );
      CHECK( max - min <= 1 ) << "min=" << min << " max=" << max;
      CHECK( sum == len );
    }


  public:
    PrefetchingThreadQueue( uint64_t prefetchDistance ) 
      : queues( new ThreadQueue[prefetchDistance] )
      , eq_index( 0 )
      , dq_index( 0 )
      , num_queues( prefetchDistance )
      , len( 0 )
  {}

    uint64_t length() {
      return len;
    }

    void enqueue(Worker * t) {
      queues[eq_index].enqueue( t );
      eq_index = ((eq_index+1) == num_queues) ? 0 : eq_index+1;
      len++;
      
#ifdef DEBUG
      check_invariants();
#endif
    }

    Worker * dequeue() {
      Worker * result = queues[dq_index].dequeue();
      if ( result ) {
        dq_index = ((dq_index+1) == num_queues) ? 0 : dq_index+1;
        len--;
#ifdef DEBUG
        check_invariants();
#endif
        return result;
      } else {
        DCHECK( dq_index == eq_index ) << "Empty queue invariant violated";
#ifdef DEBUG
        for ( uint64_t i=0; i<num_queues; i++ ) {
          CHECK( queues[i].length() == 0 ) << "Empty queue invariant violated";
        }
        check_invariants();
#endif
        return NULL;
      }
    }
};


/// Remove a Thread from the queue and return it
inline Worker * ThreadQueue::dequeue() {
    Worker * result = head;
    if (result != NULL) {
        head = result->next;
        result->next = NULL;
        len--;
    } else {
        tail = NULL;
    }
    return result;
}

/// Add a Thread to the queue
inline void ThreadQueue::enqueue( Worker * t) {
    if (head==NULL) {
        head = t;
    } else {
        tail->next = t;
    }
    tail = t;
    t->next = NULL;
    len++;
}

//FIXME
/// Prefetch some Thread close to the front of the queue
inline void ThreadQueue::prefetch() {
    Worker * result = head;
    if( result ) {
      if( result->next ) result = result->next;
//      if( result->next ) result = result->next;
//      if( result->next ) result = result->next;
//      if( result->next ) result = result->next;
      result->prefetch();
    }
}


