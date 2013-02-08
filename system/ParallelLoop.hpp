// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef PARALLEL_LOOP_HPP
#define PARALLEL_LOOP_HPP

#include "CompletionEvent.hpp"
#include "ConditionVariable.hpp"
#include "Grappa.hpp"
#include "Message.hpp"
#include "MessagePool.hpp"
#include "Tasking.hpp"
#include "Addressing.hpp"

// TODO: NOTE! These are still fairly experimental. Currently the parallel loop implementation you probably want
//       is AsynParallelFor, along with a synchronization mechanism like
//       GlobalTaskJoiner. That is what is used in current benchmarks.

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
  void forall_cores(F simd_work) {
    MessagePool<(1<<16)> pool;

    CompletionEvent ce(Grappa::cores());
    auto ce_addr = make_global(&ce);
    
    for (Core c = 0; c < Grappa::cores(); c++) {
      pool.send_message(c, [ce_addr, simd_work] {
        privateTask([ce_addr, simd_work] {
          simd_work();
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
      [loop_body](int64_t start, int64_t iters) {
        loop_body(start, iters);
      });
  }
  
  extern CompletionEvent local_ce;
  
  /// Blocking parallel for loop, spawns only private tasks. Synchronizes itself with
  /// either a given static CompletionEvent (template param) or the local builtin one.
  /// @warning { All calls to forall_here will share the same CompletionEvent by default,
  ///            so only one should be called at once per core. }
  template<CompletionEvent * CE = &local_ce, typename F = decltype(nullptr) >
  void forall_here(int64_t start, int64_t iters, F loop_body) {
    CE->enroll(iters);
    impl::loop_decomposition_private(start, iters,
      [loop_body](int64_t s, int64_t i) {
        loop_body(s, i);
        CE->complete(i);
      });
    CE->wait();
  }
  
  /// Spread iterations evenly (block-distributed) across all the cores, using recursive
  /// decomposition with private tasks. Blocks until all iterations on all cores complete.
  /// @warning { Same caveat as `forall_here`. }
  template<CompletionEvent * CE = &local_ce, typename F = decltype(nullptr)>
  void forall_global_nosteal(int64_t start, int64_t iters, F loop_body) {
    forall_cores([start,iters,loop_body]{
      range_t r = blockDist(start, start+iters, mycore(), cores());
      forall_here<CE>(r.start, r.end-r.start, loop_body);
    });
  }
  
} // namespace Grappa

#endif
