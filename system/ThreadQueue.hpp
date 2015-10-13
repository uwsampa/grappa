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

#pragma once

#include "Worker.hpp"
#include <limits>
#include <glog/logging.h>
#include <iostream>

namespace Grappa {


/// A queue of threads
class ThreadQueue {
    private:
        Worker * head;
        Worker * tail;
        uint64_t len;
        
      public:
        std::ostream& dump( std::ostream& o ) const {
            return o << "[length:" << len
                     << "; head:" << (void*)head
                     << "; tail:" << (void*)tail << "]";
        }

        ThreadQueue ( ) 
            : head ( NULL )
            , tail ( NULL )
            , len ( 0 ) { }

        void enqueue(Worker * t);
        Worker * dequeue();
        Worker * dequeueLazy();
        Worker * front() const;
        void prefetch() const;
        uint64_t length() const { 
            return len;
        }

        bool empty() {
            return (head==NULL);
        }     
};

}

template< typename T >
T _max(const T& a, const T& b) {
  return (a>b) ? a : b;
}
template< typename T >
T _min(const T& a, const T& b) {
  return (a<b) ? a : b;
}

namespace Grappa {

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
      CHECK( static_cast<int64_t>(max) - static_cast<int64_t>(min) >= 0 );
      CHECK( max - min <= 1 ) << "min=" << min << " max=" << max;
      CHECK( sum == len );
    }


  public:
    PrefetchingThreadQueue( ) 
      : queues( NULL )
      , eq_index( 0 )
      , dq_index( 0 )
      , num_queues( 0 )
      , len( 0 )
  {}
    
    void init( uint64_t prefetchDistance ) {
      queues = new ThreadQueue[prefetchDistance*2];
      num_queues = prefetchDistance*2;
    }

    uint64_t length() const {
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
      Worker * result = queues[dq_index].dequeueLazy();
      if ( result ) {
        uint64_t t = dq_index;
        dq_index = ((dq_index+1) == num_queues) ? 0 : dq_index+1;
        len--;


        // prefetch future stacks and thread objects
        // let D = prefetch distance; N = num subqueues = D*2

#ifdef WORKER_PREFETCH        
        __builtin_prefetch( result->next,   // prefetch the next thread in the current subqueue (i.e. i or i+D*2 mod N)
                               1,   // prefetch for RW
                               3 ); // prefetch with high temporal locality
        //__builtin_prefetch( ((char*)(result->next))+64, 1, 3 );
#endif
        
        // now safe to NULL
        result->next = NULL;

        uint64_t tstack = (t+(num_queues/2))%num_queues;//PERFORMANCE TODO: can optimize
        Worker * tstack_worker = queues[tstack].front(); 
        if ( tstack_worker ) {
#ifdef STACK_PREFETCH
          __builtin_prefetch( tstack_worker->stack,  // prefetch the stack that is prefetch distance away (i.e. i+D or i-1 mod N) 
                                                 1,  // prefetch for RW
                                                 3 ); // prefetch with high temporal locality
          __builtin_prefetch( ((char*)(tstack_worker->stack))+64, 1, 3 );
          __builtin_prefetch( ((char*)(tstack_worker->stack))+128, 1, 3 );
          //__builtin_prefetch( (char*)(tstack_worker->stack)+128+64, 1, 3 );
          //__builtin_prefetch( (char*)(tstack_worker->stack)+128+64+64, 1, 3 );
#endif
        }

#ifdef DEBUG
        check_invariants();
#endif
        return result;
      } else {
#ifdef DEBUG
        CHECK( dq_index == eq_index ) << "Empty queue invariant violated";
        for ( uint64_t i=0; i<num_queues; i++ ) {
          CHECK( queues[i].length() == 0 ) << "Empty queue invariant violated";
        }
        check_invariants();
#endif
        return NULL;
      }
    }

};


/// Remove a Worker from the queue and return it
inline Worker * ThreadQueue::dequeue() {
    Worker * result = head;
    if (result != NULL) {
        head = result->next;
        result->next = NULL;
        len--;          
        // lazily ignore setting the tail=NULL if now empty
    } else {
        tail = NULL;
    }
    return result;
}

/// Remove a Worker from the queue and return it.
/// Does not change the links of the returned Worker.
inline Worker * ThreadQueue::dequeueLazy() {
    Worker * result = head;
    if (result != NULL) {
        head = result->next;

        // TODO: instead just have the client call front() to get ->next

        len--;          
        // lazily ignore setting the tail=NULL if now empty
    } else {
        tail = NULL;
    }
    return result;
}

/// Add a Worker to the queue
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

/// Peek at the head of the queue
inline Worker * ThreadQueue::front() const {
  return head;
}

}

std::ostream& operator<< ( std::ostream& o, const Grappa::ThreadQueue& tq );
std::ostream& operator<< ( std::ostream& o, const Grappa::PrefetchingThreadQueue& tq );
