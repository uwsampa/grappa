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
#include "GlobalCompletionEvent.hpp"
#include "Barrier.hpp"
#include "Collective.hpp"
#include "function_traits.hpp"

/// Flag: loop_threshold
///
/// Iterations of `forall` loops *may* be run in parallel. A complete loop is decomposed into
/// separate (parallel) tasks by recursively splitting the number of iterations in half. This
/// parameter specifies the threshold where the decomposition ends, and the remaining iterations
/// are run serially in a single task. This global threshold is used by all parallel loops by
/// default, unless overridden statically as the `Threshold` template parameter in any of the
/// parallel loop functions.
DECLARE_int64(loop_threshold);


namespace Grappa {
  
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
    template< int64_t Threshold = USE_LOOP_THRESHOLD_FLAG, typename F = decltype(nullptr) >
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
        
        auto t = [rstart, riters, loop_body] {
          loop_decomposition_private<Threshold,F>(rstart, riters, loop_body);
        };
//        static_assert(sizeof(t)<=24,"privateTask too big");
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
    ///
    /// Optionally enrolls/completes spawned tasks with a GlobalCompletionEvent if specified
    /// (need to do this in decomposition because we want to enroll/complete with GCE where
    /// each task is spawned so work spawned by stolen tasks can complete locally.
    ///
    /// Note: this will add 16 bytes to the loop_body functor for loop decomposition (start
    /// niters) and synchronization, allowing the `loop_body` to have 8 bytes of
    /// user-defined storage.
    ///
    /// warning: truncates int64_t's to 48 bits--should be enough for most problem sizes.
    template< GlobalCompletionEvent * GCE = nullptr, int64_t Threshold = USE_LOOP_THRESHOLD_FLAG, typename F = decltype(nullptr) >
    void loop_decomposition_public(int64_t start, int64_t iterations, F loop_body) {
      DVLOG(4) << "< " << start << " : " << iterations << ">";
      
      if (iterations == 0) {
        return;
      } else if ((Threshold == USE_LOOP_THRESHOLD_FLAG && iterations <= FLAGS_loop_threshold)
                 || iterations <= Threshold) {
        loop_body(start, iterations);
        return;
      } else {
        // spawn right half
        int64_t rstart = start+(iterations+1)/2, riters = iterations/2;
        Core origin = mycore();
        
        // pack these 3 into 14 bytes so we still have room for a full 8-byte word from user
        // (also need to count 2 bytes from lambda overhead or something)
        struct { long rstart:48, riters:48, origin:16; } packed = { rstart, riters, origin };
        
        if (GCE) GCE->enroll();
        publicTask([packed, loop_body] {
          loop_decomposition_public<GCE,Threshold,F>(packed.rstart, packed.riters, loop_body);
          if (GCE) complete(make_global(GCE,packed.origin));
        });
        
        // left side here
        loop_decomposition_public<GCE,Threshold,F>(start, (iterations+1)/2, loop_body);
      }
    }

    
    extern CompletionEvent local_ce;
    extern GlobalCompletionEvent local_gce;
  }
  
  /// Blocking parallel for loop, spawns only private tasks. Synchronizes itself with
  /// either a given static CompletionEvent (template param) or the local builtin one.
  ///
  /// Intended to be used for a loop of local tasks, often used as a primitive (along
  /// with `on_all_cores`) in global loops.
  ///
  /// Subject to "may-parallelism", @see `loop_threshold`.
  ///
  /// @warning { All calls to forall_here will share the same CompletionEvent by default,
  ///            so only one should be called at once per core. }
  ///
  /// Also note: a single copy of `loop_body` is passed by reference to all of the child
  /// tasks, so be sure not to modify anything in the functor
  /// (TODO: figure out how to enforce this for all kinds of functors)
  template<CompletionEvent * CE = &impl::local_ce, int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG, typename F = decltype(nullptr) >
  void forall_here(int64_t start, int64_t iters, F loop_body) {
    CE->enroll(iters);
    impl::loop_decomposition_private<Threshold>(start, iters,
      // passing loop_body by ref to avoid copying potentially large functors many times in decomposition
      // also keeps task args < 24 bytes, preventing it from needing to be heap-allocated
      [&loop_body](int64_t s, int64_t n) {
        loop_body(s, n);
        CE->complete(n);
      });
    CE->wait();
  }
  
  /// Non-blocking parallel loop with privateTasks. Takes pointer to a functor which
  /// is shared by all child tasks. Enrolls tasks with specified GlobalCompletionEvent.
  ///
  /// Intended for spawning local tasks from within another GCE synchronization scheme
  /// (another parallel loop or otherwise).
  ///
  /// Note: this now takes a pointer to a functor so we can pass a pointer to the one
  /// functor to all child tasks rather than making lots of copies. Therefore, it
  /// should be alright to make the functor as large as desired.
  ///
  /// Subject to "may-parallelism", @see `loop_threshold`.
  template<GlobalCompletionEvent * GCE = &impl::local_gce, int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG, typename F = decltype(nullptr) >
  void forall_here_async(int64_t start, int64_t iters, const F * loop_ptr) {
    GCE->enroll(iters);
    
    impl::loop_decomposition_private<Threshold>(start, iters,
      // passing loop_body by ref to avoid copying potentially large functors many times in decomposition
      // also keeps task args < 24 bytes, preventing it from needing to be heap-allocated
      [loop_ptr](int64_t s, int64_t n) {
        (*loop_ptr)(s, n);
        GCE->complete(n);
      });
  }
  
  /// Non-blocking version of `forall_here`, does recursive decomposition of loop locally,
  /// but synchronization is up to you.
  ///
  /// Subject to "may-parallelism", @see `loop_threshold`.
  ///
  /// Note: this also cannot guarantee that `loop_body` will be in scope, so it passes it
  /// by copy to spawned tasks.
  /// Also uses impl::loop_decomposition_public, which adds 16 bytes to functor, so `loop_body`
  /// should be kept to 8 bytes for performance.
  template< GlobalCompletionEvent * GCE = &impl::local_gce, int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG, typename F = decltype(nullptr) >
  void forall_here_async_public(int64_t start, int64_t iters, F loop_body) {
    if (GCE) GCE->enroll();
    impl::loop_decomposition_public<GCE,Threshold>(start, iters,
      [loop_body](int64_t start, int64_t iters) {
        loop_body(start, iters);
      });
    if (GCE) GCE->complete();
  }
  
  /// Spread iterations evenly (block-distributed) across all the cores, using recursive
  /// decomposition with private tasks (so will not be load-balanced). Blocks until all
  /// iterations on all cores complete.
  ///
  /// Subject to "may-parallelism", @see `loop_threshold`.
  template<CompletionEvent * CE = &impl::local_ce, int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG, typename F = decltype(nullptr)>
  void forall_global_private(int64_t start, int64_t iters, F loop_body) {
    on_all_cores([start,iters,loop_body]{
      range_t r = blockDist(start, start+iters, mycore(), cores());
      forall_here<CE,Threshold>(r.start, r.end-r.start, loop_body);
    });
  }
  
  /// Spread iterations evenly (block-distributed) across all the cores, using recursive
  /// decomposition with public tasks (that may moved to a different core for load-balancing).
  /// Blocks until all iterations on all cores complete.
  ///
  /// Note: `loop_body` will be copied to each core and that single copied is shared by all
  /// public tasks under this GlobalCompletionEvent.
  ///
  /// Subject to "may-parallelism", @see `loop_threshold`.
  template<GlobalCompletionEvent * GCE = &impl::local_gce, int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG, typename F = decltype(nullptr)>
  void forall_global_public(int64_t start, int64_t iters, F loop_body) {
    // note: loop_decomp uses 2*8-byte values to specify range. We additionally track originating
    // core. So to avoid exceeding 3*8-byte task size, we save the loop body once on each core
    // (on `on_all_cores` task's stack) and embed the pointer to it in GCE, so each public task
    // can look it up when run, wherever it is.
    
    on_all_cores([start,iters,loop_body]{
//      GCE->reset();
      GCE->set_shared_ptr(&loop_body); // need to initialize this on all nodes before any tasks start
      
      // may as well do this before the barrier too, but it shouldn't matter
      range_t r = blockDist(start, start+iters, mycore(), cores());
      
      barrier();
      
      forall_here_async_public<GCE,Threshold>(r.start, r.end-r.start,
        [](int64_t s, int64_t n) {
          auto& loop_body = *GCE->get_shared_ptr<F>();
          loop_body(s,n);
        }
      );
      
      GCE->wait(); // keeps captured `loop_body` around for child tasks from any core to use
      GCE->set_shared_ptr(nullptr);
    });
  }
  
  /// Parallel loop over a global array. Spawned from a single core, fans out and runs
  /// tasks on elements that are local to each core.
  ///
  /// Subject to "may-parallelism", @see `loop_threshold`.
  ///
  /// Takes an optional pointer to a global static `GlobalCompletionEvent` as a template
  /// parameter to allow for programmer-specified task joining (to potentially allow
  /// more than one in flight simultaneously, though this call is itself blocking.
  ///
  /// takes a lambda/functor that operates on a range of iterations:
  ///   void(int64_t first_index, int64_t niters, T * first_element)
  ///
  /// Warning: you cannot simply increment `first_index` `niters` times and get the
  /// correct global index because a single task may span more than one block.
  template< typename T,
            GlobalCompletionEvent * GCE = &impl::local_gce,
            int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
            typename F = decltype(nullptr) >
  // type_traits magic to make this verison work for 3-arg functors
  typename std::enable_if< function_traits<F>::arity == 3,
    void >::type
  forall_localized(GlobalAddress<T> base, int64_t nelems, F loop_body) {
    static_assert(block_size % sizeof(T) == 0,
                  "forall_localized requires size of objects to evenly divide into block_size");
    
    GCE->enroll(cores()); // make sure everyone has to check in, even if they don't have any work
    Core origin = mycore();
    
    on_all_cores([base, nelems, loop_body, origin]{
//      GCE->reset();
//      barrier();
      
      T* local_base = base.localize();
      T* local_end = (base+nelems).localize();
      auto n = local_end - local_base;
      
      auto f = [loop_body, local_base, base](int64_t start, int64_t iters) {
        auto laddr = make_linear(local_base+start);
        loop_body(laddr-base, iters, local_base+start);
      };
      forall_here_async<GCE,Threshold>(0, n, &f);
      
      complete(make_global(GCE,origin));
      GCE->wait();
    });
  }
  
  
  /// Alternate version of forall_localized that takes a lambda/functor with signature:
  ///   void(int64_t index, T& element)
  /// (internally wraps this call in a loop and passes to the other version of forall_localized)
  ///
  /// This is meant to make it easy to make a loop where you don't care about amortizing
  /// anything for a single task. If you would like to do something that will be used by
  /// multiple iterations, use the other version of Grappa::forall_localized that takes a
  /// lambda that operates on a range.
  template< typename T,
            GlobalCompletionEvent * GCE = &impl::local_gce,
            int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
            typename F = decltype(nullptr) >
  // type_traits magic to make this verison work for 2-arg functors
  typename std::enable_if< function_traits<F>::arity == 2,
    void >::type
  forall_localized(GlobalAddress<T> base, int64_t nelems, F loop_body) {
    
    forall_localized(base, nelems, [loop_body,base](int64_t start, int64_t niters, T * first) {
      auto block_elems = block_size / sizeof(T);
      auto a = make_linear(first);
      auto n_to_boundary = a.block_max() - a;
      auto index = start;
      
      for (int64_t i=0; i<niters; i++,index++,n_to_boundary--) {
        if (n_to_boundary == 0) {
          index += block_elems * (cores()-1);
          n_to_boundary = block_elems;
        }
        loop_body(index, first[i]);
      }
    });
  }
  
  /// Asynchronous version of Grappa::forall_localized (enrolls with GCE, does not block).
  ///
  /// Spawns tasks to on cores that *may contain elements* of the part of a linear array
  /// from base[0]-base[nelems].
  ///
  /// loop_body functor should be of the form:
  ///   void(int64_t index, T& element)
  template< GlobalCompletionEvent * GCE = &impl::local_gce,
            typename T = decltype(nullptr),
            int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
            typename F = decltype(nullptr) >
  void forall_localized_async(GlobalAddress<T> base, int64_t nelems, F loop_body) {
    Core nc = cores();
    Core fc;
    int64_t nbytes = nelems*sizeof(T);
    GlobalAddress<int64_t> end = base+nelems;
    if (nelems > 0) { fc = 1; }

    size_t block_elems = block_size / sizeof(T);
    int64_t nfirstcore = base.block_max() - base;
    int64_t n = nelems - nfirstcore;
    
    if (n > 0) {
      int64_t nrest = n / block_elems;
      if (nrest >= nc-1) {
        fc = nc;
      } else {
        fc += nrest;
        if ((end - end.block_min()) && end.node() != base.node()) {
          fc += 1;
        }
      }
    }
    
    Core origin = mycore();
    Core start_core = base.node();
    MessagePool pool(cores() * sizeof(Message<std::function<void(GlobalAddress<T>,int64_t,F)>>));
    
    GCE->enroll(fc);
    for (Core i=0; i<fc; i++) {
      pool.send_message((start_core+i)%nc, [base,nelems,loop_body,origin] {
        // TODO: make this not heap-allocate the task.
        privateTask([base,nelems,loop_body,origin] {
          T* local_base = base.localize();
          T* local_end = (base+nelems).localize();
          size_t n = local_end - local_base;
          
          CompletionEvent ce(n);
          auto f = [base,loop_body, &ce](int64_t start, int64_t iters) {
            T* local_base = base.localize();
            auto laddr = make_linear(local_base+start);
            
            for (int64_t i=start; i<start+iters; i++) {
              // TODO: check if this is inlined and if this loop is unrollable
              loop_body(laddr-base, local_base[i]);
            }
            ce.complete(iters);
          };
          forall_here_async<GCE,Threshold>(0, n, &f);
          
          ce.wait();
          complete(make_global(GCE,origin));
        });
      });
    }
  }
  
} // namespace Grappa

#endif
