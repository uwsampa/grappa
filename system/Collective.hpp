////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
////////////////////////////////////////////////////////////////////////

#ifndef COLLECTIVE_HPP
#define COLLECTIVE_HPP

//#include "common.hpp"
#include "CompletionEvent.hpp"
#include "Message.hpp"
#include "Tasking.hpp"
#include "CountingSemaphoreLocal.hpp"
#include "Barrier.hpp"

#include <functional>
#include <algorithm>

// TODO/FIXME: use actual max message size (have Communicator be able to tell us)
const size_t MAX_MESSAGE_SIZE = 3192;

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
  return (a<b) ? a : b;
}
template< typename T >
T collective_mult(const T& a, const T& b) {
  return a*b;
}
template< typename T >
T collective_or(const T& a, const T& b) {
  return a || b;
}
template< typename T >
T collective_and(const T& a, const T& b) {
  return a && b;
}


namespace Grappa {
  
  namespace impl {
    const Core HOME_CORE = 0;
  }
  
  /// @addtogroup Collectives
  /// @{
  
  /// Call message (work that cannot block) on all cores, block until ack received from all.
  /// Like Grappa::on_all_cores() but does @a not spawn tasks on each core.
  /// Can safely be called concurrently with others.
  template<typename F>
  void call_on_all_cores(F work) {
    Core origin = mycore();
    CompletionEvent ce(cores()-1);
    
    auto lsz = [&ce,origin,work]{};
    
    for (Core c = 0; c < cores(); c++) if (c != mycore()) {
      send_heap_message(c, [&ce, origin, work] {
        work();
        send_heap_message(origin, [&ce]{ ce.complete(); });
      });
    }
    work(); // do my core's work
    ce.wait();
  }
  
  /// Spawn a private task on each core, block until all complete.
  /// To be used for any SPMD-style work (e.g. initializing globals).
  /// Also used as a primitive in Grappa system code where anything is done on all cores.
  ///
  /// @b Example:
  /// @code
  ///   int x[Grappa::cores()];
  ///   GlobalAddress<int> x_base = make_global(x);
  ///   Grappa::on_all_cores([x_base]{
  ///     Grappa::delegate::write(x_base+Grappa::mycore(), 1);
  ///   });
  /// @endcode
  template<typename F>
  void on_all_cores(F work) {
    
    CompletionEvent ce(cores());
    auto ce_addr = make_global(&ce);
    
    auto lsz = [ce_addr,work]{};
    
    for (Core c = 0; c < cores(); c++) {
      send_heap_message(c, [ce_addr, work] {
        spawn([ce_addr, work] {
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
      CompletionEvent * ce;
      T * array;
      Core elems_in = 0;
      size_t nelem;
    public:      
      /// SPMD, must be called on static/file-global object on all cores
      /// blocks until reduction is complete
      void call_allreduce(T * in_array, size_t nelem) {
        // setup everything (block to make sure HOME_CORE is done)
        this->array = in_array;
        this->nelem = nelem;
        
        size_t n_per_msg = MAX_MESSAGE_SIZE / sizeof(T);
        size_t nmsg = nelem / n_per_msg + (nelem % n_per_msg ? 1 : 0);
        auto nmsg_total = nmsg*(cores()-1);

        CompletionEvent local_ce;
        this->ce = &local_ce;
        this->ce->enroll( (mycore() == HOME_CORE) ? nmsg_total : nmsg );
        barrier();
        
        if (mycore() != HOME_CORE) {
          for (size_t k=0; k<nelem; k+=n_per_msg) {
            size_t this_nelem = std::min(n_per_msg, nelem-k);
            
            // everyone sends their contribution to HOME_CORE, last one wakes HOME_CORE
            send_heap_message(HOME_CORE, [this,k](void * payload, size_t payload_size) {
              DCHECK(mycore() == HOME_CORE);
      
              auto in_array = static_cast<T*>(payload);
              auto in_n = payload_size/sizeof(T);
              auto total = this->array+k;
      
              for (size_t i=0; i<in_n; i++) {
                total[i] = ReduceOp(total[i], in_array[i]);
              }
              DVLOG(3) << "incrementing HOME sem, now at " << ce->get_count();      
              this->ce->complete();
            }, (void*)(in_array+k), sizeof(T)*this_nelem);
          }
          
          DVLOG(3) << "about to block for " << nelem << " with sem == " << ce->get_count();           this->ce->wait();
          
        } else {
          auto nmsg_total = nmsg*(cores()-1);
          
          // home core waits until woken by last received message from other cores
          this->ce->wait();
          DVLOG(3) << "woke with sem == " << ce->get_count();
          
          // send total to everyone else and wake them
          char msg_buf[(cores()-1)*sizeof(PayloadMessage<std::function<void(decltype(this),size_t)>>)];
          for (Core c = 0; c < cores(); c++) {
            if (c != HOME_CORE) {
              // send totals back to all the other cores
              size_t n_per_msg = MAX_MESSAGE_SIZE / sizeof(T);
              for (size_t k=0; k<nelem; k+=n_per_msg) {
                size_t this_nelem = std::min(n_per_msg, nelem-k);
                send_heap_message(c, [this,k](void * payload, size_t psz){
                  auto total_k = static_cast<T*>(payload);
                  auto in_n = psz / sizeof(T);
                  for (size_t i=0; i<in_n; i++) {
                    this->array[k+i] = total_k[i];
                  }
                  this->ce->complete();
                  DVLOG(3) << "incrementing sem, now at " << ce->get_count();
                }, this->array+k, sizeof(T)*this_nelem);              
              }
            }
          }
          // once all messages are sent, HOME_CORE's task continues
        }
      }
    };
    
  } // namespace impl
  
  /// Called from SPMD context, reduces values from all cores calling `allreduce` and returns reduced
  /// values to everyone. Blocks until reduction is complete, so suffices as a global barrier.
  ///
  /// @warning May only one with a given type/op combination may be used at a time,
  ///          uses a function-private static variable.
  ///
  /// @b Example:
  /// @code
  ///   Grappa::on_all_cores([]{
  ///     int value = foo();
  ///     int total = Grappa::allreduce<int,collective_add>(value);
  ///   });
  /// @endcode
  template< typename T, T (*ReduceOp)(const T&, const T&) >
  T allreduce(T myval) {
    impl::Reduction<T>::result.reset();
    
    send_message(impl::HOME_CORE, [myval]{
      impl::collect_reduction<T,ReduceOp>(myval);
    });
    
    return impl::Reduction<T>::result.readFF();
  }
  
  /// Called from SPMD context.
  /// Do an in-place allreduce (works on arrays). All elements of the array will be 
  /// overwritten by the operation with the total from all cores.
  ///
  /// @warning May only one with a given type/op combination may be used at a time,
  ///          uses a function-private static variable.
  template< typename T, T (*ReduceOp)(const T&, const T&) >
  void allreduce_inplace(T * array, size_t nelem = 1) {
    static impl::InplaceReduction<T,ReduceOp> reducer;
    reducer.call_allreduce(array, nelem);
  }
  
  /// Called from a single task (usually user_main), reduces values from all cores onto the calling node.
  /// Blocks until reduction is complete.
  /// Safe to use any number of these concurrently.
  ///
  /// @b Example:
  /// @code
  ///   static int x;
  ///   void user_main() {
  ///     on_all_cores([]{ x = foo(); });
  ///     int total = reduce<int,collective_add>(&x);
  ///   }
  /// @endcode
  template< typename T, T (*ReduceOp)(const T&, const T&) >
  T reduce(const T * global_ptr) {
    //NOTE: this is written in a continuation passing
    //style to avoid the use of a GCE which async delegates only support
    CompletionEvent ce(cores()-1);
  
    T total = *global_ptr;
    Core origin = mycore();
    
    for (Core c=0; c<cores(); c++) {
      if (c != origin) {
        send_heap_message(c, [global_ptr, &ce, &total, origin]{
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

  /// Reduce over a symmetrically allocated object.
  /// Blocks until reduction is complete.  
  /// Safe to use any number of these concurrently.
  //
  /// @b Example:
  /// @code
  ///   void user_main() {
  ///     auto x = Grappa::symmetric_global_alloc<BlockAlignedInt>();
  ///     on_all_cores([]{ x = foo(); });
  ///     int total = reduce<int,collective_add>(x);
  ///   }
  /// @endcode
  template< typename T, T (*ReduceOp)(const T&, const T&)>
   T reduce( GlobalAddress<T> localizable ) {
     CompletionEvent ce(cores() - 1); 

     T total = *(localizable.localize());
     Core origin = mycore();

     for (Core c=0; c<cores(); c++) {
      if (c != origin) {
        send_heap_message(c, [localizable, &ce, &total, origin]{
          T val = *(localizable.localize());
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

  /// Reduce over a member of a symmetrically allocated object.
  /// The Accessor function is used to pull out the member.
  /// Blocks until reduction is complete.  
  /// Safe to use any number of these concurrently.
  ///
  /// @b Example:
  /// @code
  ///   struct BlockAlignedObj {
  ///     int x;
  ///   } GRAPPA_BLOCK_ALIGNED;
  ///   int getX(GlobalAddress<BlockAlignedObj> o) {
  ///     return o->x;
  ///   }
  ///   void user_main() {
  ///     auto x = Grappa::symmetric_global_alloc<BlockAlignedObj>();
  ///     on_all_cores([]{ x = foo(); });
  ///     int total = reduce<int,BlockedAlignedObj,collective_add,&getX>(x);
  ///   }
  /// @endcode
  template< typename T, typename P, T (*ReduceOp)(const T&, const T&), T (*Accessor)(GlobalAddress<P>)>
   T reduce( GlobalAddress<P> localizable ) {
     CompletionEvent ce(cores() - 1); 

     T total = Accessor(localizable);
     Core origin = mycore();

     for (Core c=0; c<cores(); c++) {
      if (c != origin) {
        send_heap_message(c, [localizable, &ce, &total, origin]{
          T val = Accessor(localizable);
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
  
  
  template< typename F = nullptr_t >
  auto sum_all_cores(F func) -> decltype(func()) {
    decltype(func()) total = func();
    CompletionEvent _ce(cores()-1);
    auto ce = make_global(&_ce);
    for (Core c=0; c < cores(); c++) if (c != mycore()) {
      send_heap_message(c, [ce,func,&total]{
        auto r = func();
        send_heap_message(ce.core(), [ce,r,&total]{
          total += r;
          ce->complete();
        });
      });
    }
    ce->wait();
    return total;
  }
  
  /// @}
} // namespace Grappa

#endif // COLLECTIVE_HPP


