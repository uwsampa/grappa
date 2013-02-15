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

namespace Grappa {
  
  /// Spawn a private task on each core, block until all complete.
  /// To be used for any SPMD-style work (e.g. initializing globals).
  /// Also used as a primitive in Grappa system code where anything is done on all cores.
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
    /// Declares that the loop threshold should be determined by the `loop_threshold` command-line flag.
    /// (default for loop decompositions)
    const int64_t USE_LOOP_THRESHOLD_FLAG = 0;
    
    // TODO: figure out way to put these different kinds of loop_decomposition
    // Need to template on the now-templated task spawn function.
    
    /// Does recursive loop decomposition, subdividing iterations by 2 until reaching
    /// the threshold and serializing the remaining iterations at each leaf.
    /// Note: this is an internal primitive for spawning tasks and does not synchronize on spawned tasks.
    ///
    /// This version spawns *private* tasks all the way down.
    template<int64_t Threshold = USE_LOOP_THRESHOLD_FLAG, typename F = decltype(nullptr) >
    void loop_decomposition_private(int64_t start, int64_t iterations, F loop_body) {
      DVLOG(4) << "< " << start << " : " << iterations << ">";
      DVLOG(4) << "sizeof(loop_body) = " << sizeof(loop_body);
      
      if (iterations == 0) {
        return;
      } else if ((Threshold == USE_LOOP_THRESHOLD_FLAG && iterations <= FLAGS_loop_threshold)
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
    
    /// Does recursive loop decomposition, subdividing iterations by 2 until reaching
    /// the threshold and serializing the remaining iterations at each leaf.
    /// Note: this is an internal primitive for spawning tasks and does not synchronize on spawned tasks.
    ///
    /// This version spawns *public* tasks all the way down.
    template<int64_t Threshold = USE_LOOP_THRESHOLD_FLAG, typename F = decltype(nullptr) >
    void loop_decomposition_public(int64_t start, int64_t iterations, F loop_body) {
      VLOG(1) << "< " << start << " : " << iterations << ">";
      
      if (iterations == 0) {
        return;
      } else if ((Threshold == USE_LOOP_THRESHOLD_FLAG && iterations <= FLAGS_loop_threshold)
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
  
  extern CompletionEvent local_ce;
  
  /// Blocking parallel for loop, spawns only private tasks. Synchronizes itself with
  /// either a given static CompletionEvent (template param) or the local builtin one.
  ///
  /// Intended to be used for a loop of local tasks, often used as a primitive (along
  /// with `on_all_cores`) in global loops.
  ///
  /// @warning { All calls to forall_here will share the same CompletionEvent by default,
  ///            so only one should be called at once per core. }
  ///
  /// Also note: a single copy of `loop_body` is passed by reference to all of the child
  /// tasks, so be sure not to modify anything in the functor
  /// (TODO: figure out how to enforce this for all kinds of functors)
  template<CompletionEvent * CE = &local_ce, int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG, typename F = decltype(nullptr) >
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
  
  /// Non-blocking version of `forall_here`, does recursive decomposition of loop locally,
  /// but synchronization is up to you.
  ///
  /// Note: this also cannot guarantee that `loop_body` will be in scope, so it passes it
  /// by copy to spawned tasks.
  template<typename F>
  void forall_here_async(int64_t start, int64_t iters, F loop_body) {
    impl::loop_decomposition_private(start, iters,
      [loop_body](int64_t start, int64_t iters) {
        loop_body(start, iters);
      });
  }
  
  /// Spread iterations evenly (block-distributed) across all the cores, using recursive
  /// decomposition with private tasks (so will not be load-balanced). Blocks until all
  /// iterations on all cores complete.
  ///
  /// @warning { Same caveat as `forall_here` about sharing a CompletionEvent. }
  template<CompletionEvent * CE = &local_ce, int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG, typename F = decltype(nullptr)>
  void forall_global_private(int64_t start, int64_t iters, F loop_body) {
    on_all_cores([start,iters,loop_body]{
      range_t r = blockDist(start, start+iters, mycore(), cores());
      forall_here<CE,Threshold>(r.start, r.end-r.start, loop_body);
    });
  }
  
  
  /// Parallel loop over a global array. Spawned from a single core, fans out and runs
  /// tasks on elements that are local to each core.
  ///
  /// Takes an optional pointer to a global static `CompletionEvent` as a template
  /// parameter to allow for programmer-specified task joining (to potentially allow
  /// more than one in flight simultaneously, though this call is itself blocking.
  ///
  /// TODO: asynchronous version that supports many outstanding calls to `forall_localized` (to be used in BFS)
  template< typename T,
            CompletionEvent * CE = &local_ce,
            int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
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
