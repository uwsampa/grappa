// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.


#ifndef COLLECTIVE_HPP
#define COLLECTIVE_HPP

//#include "Grappa.hpp"
//#include "ForkJoin.hpp"
#include "common.hpp"
#include "CompletionEvent.hpp"
#include "Message.hpp"
#include "MessagePool.hpp"
#include "Tasking.hpp"
#include "FullEmpty.hpp"
#include "Barrier.hpp"

#include <functional>

#define COLL_MAX &collective_max
#define COLL_MIN &collective_min
#define COLL_ADD &collective_add
#define COLL_MULT &collective_mult

template< typename T >
T collective_add(const T& a, const T& b) {
  return a+b;
}
template< typename T >
T collective_max(const T& a, const T& b) {
  return (a>b) ? a : b;
}
template< typename T >
T collective_min(const T& a, const T& b) {
  return (a>b) ? a : b;
}
template< typename T >
T collective_mult(const T& a, const T& b) {
  return a*b;
}

namespace Grappa {
  namespace impl {
    const Core HOME_CORE = 0;
  }
}

extern Thread * reducing_thread;
extern Node reduction_reported_in;

// This class is just for holding the reduction
// value in a type-safe manner
template < typename T >
class Reductions {
  private:
    Reductions() {}
  public:
    static T reduction_result;
    static T final_reduction_result;
};

template <typename T>
T Reductions<T>::reduction_result;

template <typename T> 
T Reductions<T>::final_reduction_result;

// wake the caller with the final reduction value set
template< typename T >
static void am_reduce_wake(T * val, size_t sz, void * payload, size_t psz) {
  Reductions<T>::final_reduction_result = *val;
  Grappa_wake(reducing_thread);
}

// wake the caller with the final reduction array value set
template< typename T >
static void am_reduce_array_wake(T * val, size_t sz, void * payload, size_t psz) {
  memcpy(Reductions<T*>::final_reduction_result, val, sz);
  Grappa_wake(reducing_thread);
}

// Grappa active message sent by every Node to Grappa::impl::HOME_CORE to perform reduction in one place
template< typename T, T (*Reducer)(const T&, const T&), T BaseVal>
static void am_reduce(T * val, size_t sz, void* payload, size_t psz) {
  CHECK(Grappa_mynode() == Grappa::impl::HOME_CORE);

  if (reduction_reported_in == 0) Reductions<T>::reduction_result = BaseVal;
  Reductions<T>::reduction_result = Reducer(Reductions<T>::reduction_result, *val);

  reduction_reported_in++;
  VLOG(5) << "reported_in = " << reduction_reported_in;
  if (reduction_reported_in == Grappa_nodes()) {
    reduction_reported_in = 0;
    for (Node n = 0; n < Grappa_nodes(); n++) {
      VLOG(5) << "waking " << n;
      T data = Reductions<T>::reduction_result;
      Grappa_call_on(n, &am_reduce_wake, &data);
    }
  }
}

// Grappa active message sent by every Node to Grappa::impl::HOME_CORE to perform reduction in one place
template< typename T, T (*Reducer)(const T&, const T&), T BaseVal>
static void am_reduce_array(T * val, size_t sz, void* payload, size_t psz) {
  CHECK(Grappa_mynode() == Grappa::impl::HOME_CORE);
  
  size_t nelem = sz / sizeof(T);

  if (reduction_reported_in == 0) {
    // allocate space for result
    Reductions<T*>::reduction_result = new T[nelem];
    for (size_t i=0; i<nelem; i++) Reductions<T*>::reduction_result[i] = BaseVal;
  }

  T * rarray = Reductions<T*>::reduction_result;
  for (size_t i=0; i<nelem; i++) {
    rarray[i] = Reducer(rarray[i], val[i]);
  }
  
  reduction_reported_in++;
  VLOG(5) << "reported_in = " << reduction_reported_in;
  if (reduction_reported_in == Grappa_nodes()) {
    reduction_reported_in = 0;
    for (Node n = 0; n < Grappa_nodes(); n++) {
      VLOG(5) << "waking " << n;
      Grappa_call_on(n, &am_reduce_array_wake, rarray, sizeof(T)*nelem);
    }
    delete [] Reductions<T*>::reduction_result;
  }
}

// am_reduce with no initial value
template< typename T, T (*Reducer)(const T&, const T&) >
static void am_reduce_noinit(T * val, size_t sz, void* payload, size_t psz) {
  CHECK(Grappa_mynode() == Grappa::impl::HOME_CORE);
  
  if (reduction_reported_in == 0) Reductions<T>::reduction_result = *val; // no base val
  else Reductions<T>::reduction_result = Reducer(Reductions<T>::reduction_result, *val);
  
  reduction_reported_in++;
  VLOG(5) << "reported_in = " << reduction_reported_in;
  if (reduction_reported_in == Grappa_nodes()) {
    reduction_reported_in = 0;
    for (Node n = 0; n < Grappa_nodes(); n++) {
      VLOG(5) << "waking " << n;
      T data = Reductions<T>::reduction_result;
      Grappa_call_on(n, &am_reduce_wake, &data);
    }
  }
}

/// Global reduction across all nodes, returning the completely reduced value to everyone.
/// Notes:
///  - this suffices as a global barrier across *all nodes*
///  - as such, only one instance of this can be running at a given time
///  - and it must be called by every node or deadlock will occur
///
/// @tparam T type of the reduced values
/// @tparam Reducer commutative and associative reduce function
/// @tparam BaseVal initial value, e.g. 0 for a sum
///
/// ALLNODES
template< typename T, T (*Reducer)(const T&, const T&), T BaseVal>
T Grappa_allreduce(T myval) {
  // TODO: do tree reduction to reduce amount of serialization at Node 0
  reducing_thread = CURRENT_THREAD;
  
  Grappa_call_on(Grappa::impl::HOME_CORE, &am_reduce<T,Reducer,BaseVal>, &myval);
  
  Grappa_suspend();
  
  return Reductions<T>::final_reduction_result;
}

// send one element for reduction
template< typename T, T (*Reducer)(const T&, const T&), T BaseVal>
void allreduce_one_message(T * array, size_t nelem, T * result = NULL) {
  const size_t maxn = 2048 / sizeof(T);
  CHECK( nelem <= maxn );
  // default is to overwrite original array
  if (!result) result = array;
  Reductions<T*>::final_reduction_result = result;

  // TODO: do tree reduction to reduce amount of serialization at Node 0
  reducing_thread = CURRENT_THREAD;
 
  Grappa_call_on(Grappa::impl::HOME_CORE, &am_reduce_array<T,Reducer,BaseVal>, array, sizeof(T)*nelem);
  Grappa_suspend();
}

/// Vector reduction. 
/// That is, result[i] = node0.array[i] + node1.array[i] + ... + nodeN.array[i], for all i.
/// ALLNODES
template< typename T, T (*Reducer)(const T&, const T&), T BaseVal>
void Grappa_allreduce(T * array, size_t nelem, T * result = NULL) {
  const size_t maxn = 2048 / sizeof(T);

  for (size_t i=0; i<nelem; i+=maxn) {
    size_t n = MIN(maxn, nelem-i);
    allreduce_one_message<T,Reducer,BaseVal>(array+i, n, result);
  }
}

/// Global reduction across all nodes, returning the completely reduced value to everyone.
/// This variant uses no initial value for the reduction. 
/// 
/// Notes:
///  - this suffices as a global barrier across *all nodes*
///  - as such, only one instance of this can be running at a given time
///  - and it must be called by every node or deadlock will occur
///
/// @tparam T type of the reduced values
/// @tparam Reducer commutative and associative reduce function
/// @tparam BaseVal initial value, e.g. 0 for a sum
/// 
/// ALLNODES
template< typename T, T (*Reducer)(const T&, const T&) >
T Grappa_allreduce_noinit(T myval) {
  // TODO: do tree reduction to reduce amount of serialization at Node 0
  reducing_thread = CURRENT_THREAD;
  
  Grappa_call_on(Grappa::impl::HOME_CORE, &am_reduce_noinit<T,Reducer>, &myval);
  
  Grappa_suspend();
  
  return Reductions<T>::final_reduction_result;
}

namespace Grappa {
  
  // Call message (work that cannot block) on all cores, block until ack received from all.
  template<typename F>
  void call_on_all_cores(F work) {
    Core origin = mycore();
    CompletionEvent ce(cores());
    
    auto lsz = [&ce,origin,work]{};
    MessagePool pool(cores()*(sizeof(Message<decltype(lsz)>)));
    
    for (Core c = 0; c < cores(); c++) {
      pool.send_message(c, [&ce, origin, work] {
        work();
        send_heap_message(origin, [&ce]{ ce.complete(); });
      });
    }
    ce.wait();
  }
  
  /// Spawn a private task on each core, block until all complete.
  /// To be used for any SPMD-style work (e.g. initializing globals).
  /// Also used as a primitive in Grappa system code where anything is done on all cores.
  template<typename F>
  void on_all_cores(F work) {
    
    CompletionEvent ce(cores());
    auto ce_addr = make_global(&ce);
    
    auto lsz = [ce_addr,work]{};
    MessagePool pool(cores()*(sizeof(Message<decltype(lsz)>)));
    
    for (Core c = 0; c < cores(); c++) {
      pool.send_message(c, [ce_addr, work] {
        privateTask([ce_addr, work] {
          work();
          complete(ce_addr);
        });
      });
    }
    ce.wait();
  }
  
  
  namespace impl {
    
    template<typename T>
    struct Reduction {
      static FullEmpty<T> result;
    };
    template<typename T> FullEmpty<T> Reduction<T>::result;
    
    template< typename T, T (*ReduceOp)(const T&, const T&) >
    void collect_reduction(const T& val) {
      static T total;
      static Core cores_in = 0;
      
      DCHECK(mycore() == HOME_CORE);
      
      if (cores_in == 0) {
        total = val;
      } else {
        total = ReduceOp(total, val);
      }
      
      cores_in++;
      DVLOG(4) << "cores_in: " << cores_in;
      
      if (cores_in == cores()) {
        cores_in = 0;
        T tmp_total = total;
        for (Core c = 0; c < cores(); c++) {
          send_heap_message(c, [tmp_total] {
            Reduction<T>::result.writeXF(tmp_total);
          });
        }
      }
    }
    
    template<typename T, T (*ReduceOp)(const T&, const T&) >
    class InplaceReduction {
    protected:
      FullEmpty<bool> done;
      T * array;
      Core cores_in = 0;
    public:
      
      /// SPMD, must be called on static/file-global object on all cores
      /// blocks until reduction is complete
      void call_allreduce(T * in_array, size_t nelem) {
        done.reset();
        this->array = in_array;
        barrier();
        
        if (mycore() != HOME_CORE) {
          // everyone sends their contribution to HOME_CORE, last one wakes HOME_CORE
          send_message(HOME_CORE, [this](void * payload, size_t payload_size) {
            DCHECK(mycore() == HOME_CORE);
      
            auto in_array = static_cast<T*>(payload);
            auto nelem = payload_size/sizeof(T);
      
            for (size_t i=0; i<nelem; i++) {
              this->array[i] = ReduceOp(this->array[i], in_array[i]);
            }
      
            this->cores_in++;
            if (this->cores_in == cores()-1) {
              // wake HOME_CORE's task and have it do the sending
              this->done.writeXF(true);
            }
          }, (void*)in_array, sizeof(T)*nelem);
          done.readFF();
          
        } else {
          // home core waits until woken by last received message from other cores
          done.readFF();
          
          // send total to everyone else and wake them
          char msg_buf[(cores()-1)*sizeof(PayloadMessage<decltype(this)>)];
          MessagePool pool(msg_buf, sizeof(msg_buf));
          for (Core c = 0; c < cores(); c++) {
            if (c != HOME_CORE) {
              // send totals back to all the other cores
              pool.send_message(c, [this](void * payload, size_t psz){
                auto total = static_cast<T*>(payload);
                auto nelem = psz / sizeof(T);
                for (size_t i=0; i<nelem; i++) {
                  this->array[i] = total[i];
                }
                this->done.writeXF(true);
              }, this->array, sizeof(T)*nelem);
            }
          }
          // once all messages are sent, HOME_CORE's task continues
        }
        // reset reduction object in case it gets reused
        this->cores_in = 0;
      }
    };
    
  }
  
  /// Called from SPMD context, reduces values from all cores calling `allreduce` and returns reduced
  /// values to everyone. Blocks until reduction is complete, so suffices as a global barrier.
  template< typename T, T (*ReduceOp)(const T&, const T&) >
  T allreduce(T myval) {
    impl::Reduction<T>::result.reset();
    
    send_message(impl::HOME_CORE, [myval]{
      impl::collect_reduction<T,ReduceOp>(myval);
    });
    
    return impl::Reduction<T>::result.readFF();
  }
  
  template< typename T, T (*ReduceOp)(const T&, const T&) >
  void allreduce_inplace(T * array, size_t nelem = 1) {
    static impl::InplaceReduction<T,ReduceOp> reducer;
    reducer.call_allreduce(array, nelem);
  }
  
  /// Called from a single task (usually user_main), reduces values from all cores onto the calling node.
  /// Blocks until reduction is complete.
  template< typename T, T (*ReduceOp)(const T&, const T&) >
  T reduce(T * global_ptr) {
    CompletionEvent ce(cores()-1);
    MessagePool pool(cores() * sizeof(Message<std::function<void(T*)>>));
  
    T total = *global_ptr;
    Core origin = mycore();
    
    for (Core c=0; c<cores(); c++) {
      if (c != origin) {
        pool.send_message(c, [global_ptr, &ce, &total, origin]{
          T val = *global_ptr;
          send_heap_message(origin, [val,&ce,&total] {
            total = ReduceOp(total, val);
            ce.complete();
          });
        });
      }
    }
    ce.wait();
    return total;
  }
  
}

#endif // COLLECTIVE_HPP


