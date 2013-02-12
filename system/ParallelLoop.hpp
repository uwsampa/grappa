// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef PARALLEL_LOOP_HPP
#define PARALLEL_LOOP_HPP

#include "CompletionEvent.hpp"
#include "ConditionVariable.hpp"
#include "Message.hpp"
#include "MessagePool.hpp"
#include "Tasking.hpp"
#include "Addressing.hpp"
#include "Communicator.hpp"

DECLARE_int64(loop_threshold);

//// implementations
//#include "ParallelLoop_future.hpp"
////#include "ParallelLoop_single.hpp"
//#include "parallel_loop_impls/ParallelLoop_singleSerial.hpp"
//
//#include "parallel_loop_impls/ParallelLoop_semaphore.hpp"

namespace Grappa {
  
  /// spawn a private task on all cores, block until all complete
  template<typename F>
  void on_all_cores(F work) {
    MessagePool<(1<<16)> pool;

    CompletionEvent ce(Grappa::cores());
    auto ce_addr = make_global(&ce);
    
    for (Core c = 0; c < Grappa::cores(); c++) {
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
    
    const int64_t STATIC_LOOP_THRESHOLD = 0;
    
    // TODO: figure out way to put these different kinds of loop_decomposition
    // Need to template on the now-templated task spawn function.
    
    template<int64_t Threshold = STATIC_LOOP_THRESHOLD, typename F = decltype(nullptr) >
    void loop_decomposition_private(int64_t start, int64_t iterations, F loop_body) {
      DVLOG(4) << "< " << start << " : " << iterations << ">";
      DVLOG(4) << "sizeof(loop_body) = " << sizeof(loop_body);
      
      if (iterations == 0) {
        return;
      } else if ((Threshold == STATIC_LOOP_THRESHOLD && iterations <= FLAGS_loop_threshold)
                 || iterations <= Threshold) {
        loop_body(start, iterations);
        return;
      } else {
        // spawn right half
        int64_t rstart = start+(iterations+1)/2, riters = iterations/2;
        
        // Ugly, but switch should be compiled away and this is easier than
        // templating on a templated function
        auto t = [rstart, riters, loop_body] {
          loop_decomposition_private<Threshold,F>(rstart, riters, loop_body);
        };
        privateTask(t);
        
        // left side here
        loop_decomposition_private<Threshold,F>(start, (iterations+1)/2, loop_body);
      }
    }
    
    template<int64_t Threshold = STATIC_LOOP_THRESHOLD, typename F = decltype(nullptr) >
    void loop_decomposition_public(int64_t start, int64_t iterations, F loop_body) {
      VLOG(1) << "< " << start << " : " << iterations << ">";
      
      if (iterations == 0) {
        return;
      } else if ((Threshold == STATIC_LOOP_THRESHOLD && iterations <= FLAGS_loop_threshold)
                 || iterations <= Threshold) {
        loop_body(start, iterations);
        return;
      } else {
        // spawn right half
        int64_t rstart = start+(iterations+1)/2, riters = iterations/2;
        
        // Ugly, but switch should be compiled away and this is easier than
        // templating on a templated function
        publicTask([rstart, riters, loop_body] {
          loop_decomposition_public<Threshold,F>(rstart, riters, loop_body);
        });
        
        // left side here
        loop_decomposition_public<Threshold,F>(start, (iterations+1)/2, loop_body);
      }
    }


  }
  
  /// Does recursive decomposition of loop, but synchronization is up to you.
  template<typename F>
  void forall_here_async(int64_t start, int64_t iters, F loop_body) {
    impl::loop_decomposition_private(start, iters,
      [&loop_body](int64_t start, int64_t iters) {
        loop_body(start, iters);
      });
  }
  
  extern CompletionEvent local_ce;
  
  /// Blocking parallel for loop, spawns only private tasks. Synchronizes itself with
  /// either a given static CompletionEvent (template param) or the local builtin one.
  /// @warning { All calls to forall_here will share the same CompletionEvent by default,
  ///            so only one should be called at once per core. }
  ///
  /// Also note: a single copy of `loop_body` is passed by reference to all of the child
  /// tasks, so be sure not to modify anything in the functor
  /// (TODO: figure out how to enforce this for all kinds of functors)
  template<CompletionEvent * CE = &local_ce, int64_t Threshold = impl::STATIC_LOOP_THRESHOLD, typename F = decltype(nullptr) >
  void forall_here(int64_t start, int64_t iters, F loop_body) {
    CE->enroll(iters);
    impl::loop_decomposition_private<Threshold>(start, iters,
      // passing loop_body by ref to avoid copying potentially large functors many times in decomposition
      // also keeps task args < 24 bytes, preventing it from being needing to be heap-allocated
      [&loop_body](int64_t s, int64_t i) {
        loop_body(s, i);
        CE->complete(i);
      });
    CE->wait();
  }
  
  /// Spread iterations evenly (block-distributed) across all the cores, using recursive
  /// decomposition with private tasks. Blocks until all iterations on all cores complete.
  /// @warning { Same caveat as `forall_here`. }
  template<CompletionEvent * CE = &local_ce, int64_t Threshold = impl::STATIC_LOOP_THRESHOLD, typename F = decltype(nullptr)>
  void forall_global_nosteal(int64_t start, int64_t iters, F loop_body) {
    on_all_cores([start,iters,loop_body]{
      range_t r = blockDist(start, start+iters, mycore(), cores());
      forall_here<CE,Threshold>(r.start, r.end-r.start, loop_body);
    });
  }
  
  
  template< typename T,
            CompletionEvent * CE = &local_ce,
            int64_t Threshold = impl::STATIC_LOOP_THRESHOLD,
            typename F = decltype(nullptr) >
  void forall_localized(GlobalAddress<T> base, int64_t nelems, F loop_body) {
    on_all_cores([base, nelems, loop_body]{
      T* local_base = base.localize();
      T* local_end = (base+nelems).localize();
      
      forall_here<CE,Threshold>(0, local_end-local_base,
        // note: this functor will be passed by ref in `forall_here` so should be okay if >24B
        [loop_body, local_base, base](int64_t start, int64_t iters) {
          auto laddr = make_linear(local_base+start);
          
          for (int64_t i=start; i<start+iters; i++) {
            // TODO: check if this is inlined and if this loop is unrollable
            loop_body(laddr-base, local_base[i]);
          }
        }
      );
    });
  }
  
} // namespace Grappa

#endif
