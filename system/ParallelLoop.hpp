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
  /// @addtogroup Loops
  /// @{
  
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
    template< GlobalCompletionEvent * GCE, int64_t Threshold = USE_LOOP_THRESHOLD_FLAG, typename F = decltype(nullptr) >
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
    
    template<CompletionEvent * CE, int64_t Threshold, typename F >
    void forall_here(int64_t start, int64_t iters, F loop_body, void (F::*mf)(int64_t,int64_t) const) {
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

    template<CompletionEvent * CE, int64_t Threshold, typename F >
    void forall_here(int64_t start, int64_t iters, F loop_body, void (F::*mf)(int64_t) const) {
      auto f = [loop_body](int64_t s, int64_t n) {
        for (int64_t i=s; i<s+n; i++) {
          loop_body(i);
        }
      };
      forall_here<CE,Threshold>(start, iters, f, &decltype(f)::operator());
    }
    
    extern CompletionEvent local_ce;
    extern GlobalCompletionEvent local_gce;
  } // namespace impl
  
  /// Blocking parallel for loop, spawns only private tasks. Synchronizes itself with
  /// either a given static CompletionEvent (template param) or the local builtin one.
  ///
  /// Intended to be used for a loop of local tasks, often used as a primitive (along
  /// with `on_all_cores`) in global loops.
  ///
  /// Subject to "may-parallelism", @see `loop_threshold`.
  ///
  /// @warning All calls to forall_here will share the same CompletionEvent by default,
  ///          so only one should be called at once per core.
  ///
  /// Also note: a single copy of `loop_body` is passed by reference to all of the child
  /// tasks, so be sure not to modify anything in the functor
  /// (TODO: figure out how to enforce this for all kinds of functors)
  ///
  /// Example usage:
  /// @code
  ///   for (int i=0; i<N; i++) { x++; }
  ///   // equivalent parallel loop:
  ///   forall_here(0, N, [&x](int64_t start, int64_t iters) {
  ///     for (int i=start; i<start+iters; i++) { x++; }
  ///   });
  /// @endcode
  template<CompletionEvent * CE = &impl::local_ce, int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG, typename F = decltype(nullptr) >
  inline void forall_here(int64_t start, int64_t iters, F loop_body) {
    impl::forall_here<CE,Threshold,F>(start, iters, loop_body, &decltype(loop_body)::operator());
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
  template<GlobalCompletionEvent * GCE = nullptr, int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG, typename F = decltype(nullptr) >
  void forall_here_async(int64_t start, int64_t iters, F loop_body) {
    if (GCE) GCE->enroll(iters);
    
    struct HeapF {
      const F loop_body;
      int64_t refs;
      HeapF(F loop_body, int64_t refs): loop_body(std::move(loop_body)), refs(refs) {}
      void ref(int64_t delta) {
        refs += delta;
        if (refs == 0) delete this;
      }
    };
    auto hf = new HeapF(loop_body, iters);
    
    impl::loop_decomposition_private<Threshold>(start, iters,
      // passing loop_body by ref to avoid copying potentially large functors many times in decomposition
      // also keeps task args < 24 bytes, preventing it from needing to be heap-allocated
      [hf](int64_t s, int64_t n) {
        hf->loop_body(s, n);
        if(GCE) GCE->complete(n);
        hf->ref(-n);
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
  
  namespace impl {
  
    template<GlobalCompletionEvent * GCE, int64_t Threshold, typename F>
    void forall_global_public(int64_t start, int64_t iters, F loop_body,
                                void (F::*mf)(int64_t,int64_t) const)
    {
      // note: loop_decomp uses 2*8-byte values to specify range. We additionally track originating
      // core. So to avoid exceeding 3*8-byte task size, we save the loop body once on each core
      // (on `on_all_cores` task's stack) and embed the pointer to it in GCE, so each public task
      // can look it up when run, wherever it is.
    
      on_all_cores([start,iters,loop_body]{
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
  
    template<GlobalCompletionEvent * GCE, int64_t Threshold, typename F>
    void forall_global_public(int64_t start, int64_t iters, F loop_body,
                              void (F::*mf)(int64_t) const)
    {
      auto f = [loop_body](int64_t start, int64_t iters){
        for (int64_t i=start; i<start+iters; i++) {
          loop_body(i);
        }
      };
      impl::forall_global_public<GCE,Threshold>(start, iters, f, &decltype(f)::operator());
    }
  }
  
  /// Spread iterations evenly (block-distributed) across all the cores, using recursive
  /// decomposition with public tasks (that may moved to a different core for load-balancing).
  /// Blocks until all iterations on all cores complete.
  ///
  /// Note: `loop_body` will be copied to each core and that single copy is shared by all
  /// public tasks under this GlobalCompletionEvent.
  ///
  /// Subject to "may-parallelism", @see `loop_threshold`.
  template<GlobalCompletionEvent * GCE = &impl::local_gce, int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG, typename F = decltype(nullptr)>
  void forall_global_public(int64_t start, int64_t iters, F loop_body) {
    impl::forall_global_public<GCE,Threshold>(start, iters, loop_body, &F::operator());
  }
  
  namespace impl {
  
    template< GlobalCompletionEvent * GCE, int64_t Threshold, typename T, typename F >
    void forall_localized(GlobalAddress<T> base, int64_t nelems, F loop_body,
                          void (F::*mf)(int64_t,int64_t,T*) const)
    {
      static_assert(block_size % sizeof(T) == 0,
                    "forall_localized requires size of objects to evenly divide into block_size");
    
      GCE->enroll(cores()); // make sure everyone has to check in, even if they don't have any work
      Core origin = mycore();
    
      on_all_cores([base, nelems, loop_body, origin]{      
        T* local_base = base.localize();
        T* local_end = (base+nelems).localize();
        auto n = local_end - local_base;
      
        auto f = [loop_body, local_base, base](int64_t start, int64_t iters) {
          auto laddr = make_linear(local_base+start);
          loop_body(laddr-base, iters, local_base+start);
        };
        forall_here_async<GCE,Threshold>(0, n, f);
      
        complete(make_global(GCE,origin));
        GCE->wait();
      });
    }
  
    template< GlobalCompletionEvent * GCE, int64_t Threshold, typename T, typename F >
    void forall_localized(GlobalAddress<T> base, int64_t nelems, F loop_body,
                          void (F::*mf)(int64_t,T&) const)
    {
      auto f = [loop_body](int64_t start, int64_t niters, T * first) {
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
      };
      impl::forall_localized<GCE,Threshold,T>(base, nelems, f, &decltype(f)::operator());
    }

    template< GlobalCompletionEvent * GCE, int64_t Threshold, typename T, typename F >
    void forall_localized(GlobalAddress<T> base, int64_t nelems, F loop_body,
                          void (F::*mf)(T&) const)
    {
      auto f = [loop_body](int64_t start, int64_t niters, T * first) {
        for (int64_t i=0; i<niters; i++) {
          loop_body(first[i]);
        }
      };
      impl::forall_localized<GCE,Threshold,T>(base, nelems, f, &decltype(f)::operator());
    }

  } // namespace impl
  
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
  /// @warning You cannot simply increment `first_index` `niters` times and get the
  /// correct global index because a single task may span more than one block.
  ///
  /// Example:
  /// @code
  ///   // GlobalCompletionEvent gce declared in global scope
  ///   GlobalAddress<double> array, dest;
  ///   forall_localized<&gce>(array, N, [dest](int64_t start, int64_t niters, double * first){
  ///     for (int64_t i=0; i<niters; i++) {
  ///       delegate::write_async<&gce>(shared_pool, dest+start+i, 2.0*first+i);
  ///     }
  ///   });
  /// @endcode
  ///
  /// Alternatively, forall_localized can take a lambda/functor with signature:
  ///   void(int64_t index, T& element)
  /// (internally wraps this call in a loop and passes to the other version of forall_localized)
  ///
  /// This is meant to make it easy to make a loop where you don't care about amortizing
  /// anything for a single task. If you would like to do something that will be used by
  /// multiple iterations, use the other version of Grappa::forall_localized that takes a
  /// lambda that operates on a range.
  ///
  /// Example:
  /// @code
  ///   // GlobalCompletionEvent gce declared in global scope
  ///   GlobalAddress<double> array, dest;
  ///   forall_localized<&gce>(array, N, [dest](int64_t i, double& v){
  ///     delegate::write_async<&gce>(shared_pool, dest+i, 2.0*v);
  ///   });
  /// @endcode
  template< GlobalCompletionEvent * GCE = &impl::local_gce,
            int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
            typename T = decltype(nullptr),
            typename F = decltype(nullptr) >
  void forall_localized(GlobalAddress<T> base, int64_t nelems, F loop_body) {
    impl::forall_localized<GCE,Threshold,T,F>(base, nelems, loop_body, &F::operator());
  }

  /// Overload to allow using default GCE but specifying threshold
  template< int64_t Threshold, typename T = decltype(nullptr), typename F = decltype(nullptr) >
  void forall_localized(GlobalAddress<T> base, int64_t nelems, F loop_body) {
    impl::forall_localized<&impl::local_gce,Threshold,T,F>(base, nelems, loop_body, &F::operator());
  }
  
  /// Return range of cores that have elements for the given linear address range.
  template< typename T >
  std::pair<Core,Core> cores_with_elements(GlobalAddress<T> base, size_t nelem) {
    Core start_core = base.node();
    Core nc = cores();
    Core fc;
    int64_t nbytes = nelem*sizeof(T);
    GlobalAddress<int64_t> end = base+nelem;
    if (nelem > 0) { fc = 1; }
  
    size_t block_elems = block_size / sizeof(T);
    int64_t nfirstcore = base.block_max() - base;
    int64_t n = nelem - nfirstcore;
    
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
    
    return std::make_pair( start_core, fc );
  }
  
  /// Run privateTasks on each core that contains elements of the given region of global memory.
  /// do_on_core: void(T* local_base, size_t nlocal)
  /// Internally creates privateTask with 2*8-byte words, so do_on_core can be 8 bytes and not cause heap allocation.
  template< GlobalCompletionEvent * GCE = &impl::local_gce,
            int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
            typename T = decltype(nullptr),
            typename F = decltype(nullptr) >
  void on_cores_localized_async(GlobalAddress<T> base, int64_t nelems, F do_on_core) {
    
    auto r = cores_with_elements(base, nelems);
    auto fc = r.second;
    auto nc = cores();
    
    Core origin = mycore();
    MessagePool pool(cores() * sizeof(Message<std::function<void(GlobalAddress<T>,int64_t,F)>>));
    
    GCE->enroll(fc);
    struct { int64_t nelems : 48, origin : 16; } packed = { nelems, mycore() };
    
    for (Core i=0; i<fc; i++) {
      pool.send_message((r.first+i)%nc, [base,packed,do_on_core] {
        privateTask([base,packed,do_on_core] {
          T* local_base = base.localize();
          T* local_end = (base+packed.nelems).localize();
          size_t n = local_end - local_base;
          do_on_core(local_base, n);
          complete(make_global(GCE,packed.origin));
        });
      });
    }
  }

  namespace impl {
    template< GlobalCompletionEvent * GCE, int64_t Threshold, typename T, typename F >
    void forall_localized_async(GlobalAddress<T> base, int64_t nelems, F loop_body,
                                void (F::*mf)(int64_t,int64_t,T*) const)
    {
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
    
      struct { int64_t nelems:48, origin:16; } pack = { nelems, mycore() };
    
      if (GCE) GCE->enroll(fc);
      for (Core i=0; i<fc; i++) {
        pool.send_message((start_core+i)%nc, [base,pack,loop_body] {
          // (should now have room for loop_body to be 8 bytes)
          privateTask([base,pack,loop_body] {
            T* local_base = base.localize();
            T* local_end = (base+pack.nelems).localize();
            size_t n = local_end - local_base;
          
            auto f = [base,loop_body](int64_t start, int64_t iters) {
              T* local_base = base.localize();
              auto laddr = make_linear(local_base+start);
            
              loop_body(laddr-base, iters, local_base+start);
            };
            forall_here_async<GCE,Threshold>(0, n, f);
          
            if (GCE) complete(make_global(GCE,pack.origin));
          });
        });
      }
    }
  
    template< GlobalCompletionEvent * GCE, int64_t Threshold, typename T, typename F >  
    void forall_localized_async(GlobalAddress<T> base, int64_t nelems, F loop_body,
                                void (F::*mf)(int64_t,T&) const)
    {
      auto f = [loop_body](int64_t start, int64_t niters, T* first){
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
      };
      impl::forall_localized_async<GCE,Threshold>(base, nelems, f, &decltype(f)::operator());
    }
  
    template< GlobalCompletionEvent * GCE, int64_t Threshold, typename T, typename F >  
    void forall_localized_async(GlobalAddress<T> base, int64_t nelems, F loop_body,
                                void (F::*mf)(T&) const)
    {
      auto f = [loop_body](int64_t start, int64_t niters, T * first) {
        for (int64_t i=0; i<niters; i++) {
          loop_body(first[i]);
        }
      };
      impl::forall_localized_async<GCE,Threshold>(base, nelems, f, &decltype(f)::operator());
    }
  
  
  } // namespace impl
  
  /// Asynchronous version of Grappa::forall_localized (enrolls with GCE, does not block).
  ///
  /// Spawns tasks to on cores that *may contain elements* of the part of a linear array
  /// from base[0]-base[nelems].
  ///
  /// loop_body functor should be of the form:
  /// @code
  ///   void(int64_t index, T& element)
  /// @endcode
  template< GlobalCompletionEvent * GCE = &impl::local_gce,
            int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
            typename T = decltype(nullptr),
            typename F = decltype(nullptr) >
  void forall_localized_async(GlobalAddress<T> base, int64_t nelems, F loop_body) {
    impl::forall_localized_async<GCE,Threshold>(base, nelems, loop_body, &F::operator());
  }
  
  /// @}
  
} // namespace Grappa

#endif
