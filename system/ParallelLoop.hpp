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
#ifndef PARALLEL_LOOP_HPP
#define PARALLEL_LOOP_HPP

#include "CompletionEvent.hpp"
#include "ConditionVariable.hpp"
#include "Message.hpp"
#include "Tasking.hpp"
#include "Addressing.hpp"
#include "Communicator.hpp"
#include "GlobalCompletionEvent.hpp"
#include "Barrier.hpp"
#include "Collective.hpp"
#include "function_traits.hpp"

#include <algorithm>

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
    
  } // namespace impl
  
  inline GlobalCompletionEvent& default_gce() { return local_gce; }
    
  namespace impl {
    
    /// Does recursive loop decomposition, subdividing iterations by 2 until reaching
    /// the threshold and serializing the remaining iterations at each leaf.
    /// Note: this is an internal primitive for spawning tasks and does not synchronize on spawned tasks.
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
    template< TaskMode B,
              GlobalCompletionEvent * C = nullptr,
              int64_t Threshold = USE_LOOP_THRESHOLD_FLAG,
              typename F = decltype(nullptr) >
    void loop_decomposition(int64_t start, int64_t iterations, F loop_body) {
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
        
        if (C) C->enroll();
        spawn<B>([packed, loop_body] {
          loop_decomposition<B,C,Threshold>(packed.rstart, packed.riters, loop_body);
          if (C) C->send_completion(packed.origin);
        });
        
        // left side here
        loop_decomposition<B,C,Threshold>(start, (iterations+1)/2, loop_body);
      }
    }

    template< TaskMode B, int64_t Threshold,
              GlobalCompletionEvent * C = nullptr,
              typename F = decltype(nullptr) >
    void loop_decomposition(int64_t start, int64_t iterations, F loop_body) {
      loop_decomposition<B,C,Threshold>(start, iterations, loop_body);
    }
    
  }
    
  namespace impl {
    
    template<TaskMode B, SyncMode S, GlobalCompletionEvent * C, int64_t Threshold, typename F>
    void forall_here(int64_t start, int64_t iters, F loop_body,
                     void (F::*mf)(int64_t,int64_t) const)
    {
      struct HeapF {
       const F loop_body;
       int64_t refs;
       HeapF(F loop_body, int64_t refs): loop_body(std::move(loop_body)), refs(refs) {}
       void ref(int64_t delta) {
         refs += delta;
         if (refs == 0) delete this;
       }
      };
      
      if (C == nullptr && S == SyncMode::Blocking) {
        if (B == TaskMode::Bound) {
          CompletionEvent ce(iters);
          impl::loop_decomposition<B,C,Threshold>(start, iters,
          [&loop_body,&ce](int64_t s, int64_t n){
            loop_body(s, n);
            ce.complete(n);
          });
          ce.wait();
        } else {
          CHECK(false) << "unimplemented, sorry!";
        }
      } else if (C && S == SyncMode::Async && B == TaskMode::Bound
          && sizeof(F) > 8
          && C->get_shared_ptr<F>() == nullptr) {
        auto hf = new HeapF(loop_body, iters);
        impl::loop_decomposition<B,C,Threshold>(start, iters,
            [hf](int64_t s, int64_t n){
          hf->loop_body(s, n);
          hf->ref(-n);
        });
      } else {
        impl::loop_decomposition<B,C,Threshold>(start, iters, loop_body);
        if (S == SyncMode::Blocking && C) C->wait();
      }
    }
    
    template<TaskMode B, SyncMode S, GlobalCompletionEvent * C, int64_t Threshold, typename F>
    void forall_here(int64_t start, int64_t iters, F loop_body,
                     void (F::*mf)(int64_t) const)
    {
      auto f = [loop_body](int64_t s, int64_t n){
        for (int64_t i=0; i < n; i++) {
          loop_body(s+i);
        }
      };
      impl::forall_here<B,S,C,Threshold>(start, iters, f, &decltype(f)::operator());
    }
    
    template<TaskMode B, SyncMode S, GlobalCompletionEvent * C, int64_t Threshold, typename F>
    void forall_here(int64_t start, int64_t iters, F loop_body) {
      forall_here<B,S,C,Threshold>(start, iters, loop_body, &F::operator());
    }
    
  }
  
  template< SyncMode S = SyncMode::Blocking,
            TaskMode B = TaskMode::Bound,
            GlobalCompletionEvent * GCE = nullptr,
            int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
            typename F = decltype(nullptr) >
  void forall_here(int64_t start, int64_t iters, F loop_body) {
    impl::forall_here<B,S,GCE,Threshold>(start, iters, loop_body);
  }
  
  /// Overload
#define FORALL_HERE_OVERLOAD(...)  \
  template< __VA_ARGS__, typename F = decltype(nullptr) > \
  void forall_here(int64_t start, int64_t iters, F loop_body) { \
    impl::forall_here<B,S,GCE,Threshold>(start, iters, loop_body); \
  }

  FORALL_HERE_OVERLOAD(TaskMode B,
                       SyncMode S = SyncMode::Blocking,
                       GlobalCompletionEvent * GCE = nullptr,
                       int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG);

  FORALL_HERE_OVERLOAD(SyncMode S,
                       GlobalCompletionEvent * GCE,
                       TaskMode B = TaskMode::Bound,
                       int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG);
                       
  FORALL_HERE_OVERLOAD(SyncMode S,
                       GlobalCompletionEvent * GCE,
                       int64_t Threshold,
                       TaskMode B = TaskMode::Bound);
                  
  FORALL_HERE_OVERLOAD(GlobalCompletionEvent * GCE,
                       int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
                       TaskMode B = TaskMode::Bound,
                       SyncMode S = SyncMode::Blocking);
  
  FORALL_HERE_OVERLOAD(int64_t Threshold,
                       GlobalCompletionEvent * GCE = nullptr,
                       TaskMode B = TaskMode::Bound,
                       SyncMode S = SyncMode::Blocking);
  
#undef FORALL_HERE_OVERLOAD
  
  namespace impl {
  
    template< TaskMode B, SyncMode S, GlobalCompletionEvent * C, int64_t Threshold, typename F >
    void forall(int64_t start, int64_t iters, F loop_body,
        void (F::*mf)(int64_t,int64_t) const) {
      static_assert( C != nullptr, "GCE template arg cannot be null; need the GCE to store shared args");
    
      Core origin = mycore();
      C->enroll(cores());
      
      on_all_cores([start,iters,origin,loop_body]{
        
        range_t r = blockDist(start, start+iters, mycore(), cores());
        
        if (sizeof(loop_body) > 8) {
          
          CHECK(S != SyncMode::Async) << "Cannot do 'async' with a lambda > 8 bytes.";
          
          // initialize this on all cores before starting any tasks
          C->set_shared_ptr(&loop_body);
          barrier();
          
          forall_here<B,SyncMode::Async,C,Threshold>(r.start, r.end-r.start, [](int64_t s, int64_t n){
            auto& loop_body = *C->get_shared_ptr<F>();
            loop_body(s,n);
          });
        
          C->send_completion(origin);
          C->wait();
          C->set_shared_ptr(nullptr);
          
        } else {
          
          forall_here<B,SyncMode::Async,C,Threshold>(r.start, r.end-r.start, loop_body);
          C->send_completion(origin);
          if (S == SyncMode::Blocking) C->wait();
          
        }
      });    
    }
    
    // Convert lambda to `void(i64,i64)`
    template< TaskMode B, SyncMode S, GlobalCompletionEvent * C, int64_t Threshold, typename F >
    void forall(int64_t start, int64_t iters, F loop_body, void (F::*mf)(int64_t) const) {
      auto f = [loop_body](int64_t s, int64_t n){
        for (int64_t i=0; i < n; i++) {
          loop_body(s+i);
        }
      };
      impl::forall<B,S,C,Threshold>(start, iters, f, &decltype(f)::operator());
    }
  }
  
#define FORALL_OVERLOAD(...) \
  template< __VA_ARGS__, typename F = decltype(nullptr) > \
  void forall(int64_t start, int64_t iters, F loop_body) { \
    impl::forall<B,S,C,Threshold>(start, iters, loop_body, &F::operator()); \
  }
  
  FORALL_OVERLOAD(TaskMode B = TaskMode::Bound,
                  SyncMode S = SyncMode::Blocking,
                  GlobalCompletionEvent * C = &impl::local_gce,
                  int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG);

  FORALL_OVERLOAD(SyncMode S,
                  TaskMode B = TaskMode::Bound,
                  GlobalCompletionEvent * C = &impl::local_gce,
                  int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG);
  
  FORALL_OVERLOAD(GlobalCompletionEvent * C,
                  int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
                  TaskMode B = TaskMode::Bound,
                  SyncMode S = SyncMode::Blocking);

  FORALL_OVERLOAD(int64_t Threshold,
                  GlobalCompletionEvent * C = &impl::local_gce,
                  TaskMode B = TaskMode::Bound,
                  SyncMode S = SyncMode::Blocking);
                  
  FORALL_OVERLOAD(TaskMode B,
                  GlobalCompletionEvent * C,
                  SyncMode S = SyncMode::Blocking,
                  int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG);
                  
  
#undef FORALL_OVERLOAD
    
  namespace impl {
    
    template< GlobalCompletionEvent * GCE, int64_t Threshold, typename T, typename F >
    void forall(GlobalAddress<T> base, int64_t nelems, F loop_body,
                          void (F::*mf)(T&) const)
    {
      auto f = [loop_body](int64_t start, int64_t niters, T * first) {
        for (int64_t i=0; i<niters; i++) {
          loop_body(first[i]);
        }
      };
      impl::forall<GCE,Threshold,T>(base, nelems, f, &decltype(f)::operator());
    }

  } // namespace impl
    
  /// Return range of cores that have elements for the given linear address range.
  template< typename T >
  std::pair<Core,Core> cores_with_elements(GlobalAddress<T> base, size_t nelem) {
    Core start_core = base.core();
    Core nc = cores();
    Core fc;
    int64_t nbytes = nelem*sizeof(T);
    auto end = base+nelem;
    if (nelem > 0) { fc = 1; }
  
    size_t block_elems = std::max<size_t>(1, block_size / sizeof(T));
    int64_t nfirstcore = base.block_max() - base;
    int64_t n = nelem - nfirstcore;
    
    if (n > 0) {
      int64_t nrest = n / block_elems;
      if (nrest >= nc-1) {
        fc = nc;
      } else {
        fc += nrest;
        if ((end - end.block_min()) && end.core() != base.core()) {
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
    
    if (GCE) GCE->enroll(fc);
    struct { int64_t nelems : 48, origin : 16; } packed = { nelems, mycore() };
    
    for (Core i=0; i<fc; i++) {
      send_heap_message((r.first+i)%nc, [base,packed,do_on_core] {
        spawn([base,packed,do_on_core] {
          T* local_base = base.localize();
          T* local_end = (base+packed.nelems).localize();
          size_t n = local_end - local_base;
          do_on_core(local_base, n);
          if (GCE) complete(make_global(GCE,packed.origin));
        });
      });
    }
  }

  namespace impl {
    template< TaskMode B, SyncMode S,
              GlobalCompletionEvent * GCE, int64_t Threshold,
              typename T, typename F >
    void forall(GlobalAddress<T> base, int64_t nelems, F loop_body,
                                void (F::*mf)(int64_t,int64_t,T*) const)
    {
      static_assert( B == TaskMode::Bound,
                     "balancing tasks with localized forall not supported yet" );      
      
      on_cores_localized_async<GCE,Threshold>(base, nelems,
      [loop_body,base](T* local_base, size_t nlocal){
        Grappa::forall_here<B,SyncMode::Async,GCE,Threshold>(0, nlocal, 
            [loop_body,local_base,base](int64_t s, int64_t n){
          loop_body( make_linear(local_base+s)-base, n, local_base+s );
        });
      });
      
      if (S == SyncMode::Blocking && GCE) GCE->wait();
    }
  
    template< TaskMode B, SyncMode S,
              GlobalCompletionEvent * GCE, int64_t Threshold,
              typename T, typename F >
    void forall(GlobalAddress<T> base, int64_t nelems, F loop_body,
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
      impl::forall<B,S,GCE,Threshold>(base, nelems, f, &decltype(f)::operator());
    }
  
    template< TaskMode B, SyncMode S,
              GlobalCompletionEvent * GCE, int64_t Threshold,
              typename T, typename F >
    void forall(GlobalAddress<T> base, int64_t nelems, F loop_body,
                void (F::*mf)(T&) const)
    {
      auto f = [loop_body](int64_t start, int64_t niters, T * first) {
        for (int64_t i=0; i<niters; i++) {
          loop_body(first[i]);
        }
      };
      impl::forall<B,S,GCE,Threshold>(base, nelems, f, &decltype(f)::operator());
    }
    
  
  } // namespace impl
  

  /// Parallel loop over a global array. Spawned from a single core, fans out and runs
  /// tasks on elements that are local to each core.
  ///
  /// Subject to "may-parallelism", @see `loop_threshold`.
  ///
  /// Takes an optional pointer to a global static `GlobalCompletionEvent` as a template
  /// parameter to allow for programmer-specified task joining (to potentially allow
  /// more than one in flight simultaneously, though this call is itself sync.
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
  ///   forall<&gce>(array, N, [dest](int64_t start, int64_t niters, double * first){
  ///     for (int64_t i=0; i<niters; i++) {
  ///       delegate::write<async,&gce>(dest+start+i, 2.0*first+i);
  ///     }
  ///   });
  /// @endcode
  ///
  /// Alternatively, forall can take a lambda/functor with signature:
  ///   void(int64_t index, T& element)
  /// (internally wraps this call in a loop and passes to the other version of forall)
  ///
  /// This is meant to make it easy to make a loop where you don't care about amortizing
  /// anything for a single task. If you would like to do something that will be used by
  /// multiple iterations, use the other version of Grappa::forall that takes a
  /// lambda that operates on a range.
  ///
  /// Example:
  /// @code
  ///   // GlobalCompletionEvent gce declared in global scope
  ///   GlobalAddress<double> array, dest;
  ///   forall<&gce>(array, N, [dest](int64_t i, double& v){
  ///     delegate::write<async,&gce>(dest+i, 2.0*v);
  ///   });
  /// @endcode
  template< TaskMode B = TaskMode::Bound,
            SyncMode S = SyncMode::Blocking,
            GlobalCompletionEvent * GCE = &impl::local_gce,
            int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
            typename T = decltype(nullptr),
            typename F = decltype(nullptr) >
  void forall(GlobalAddress<T> base, int64_t nelems, F loop_body) {
    impl::forall<B,S,GCE,Threshold>(base, nelems, loop_body, &F::operator());
  }

  /// Overload for specifying just SyncMode (or SyncMode first)
  template< SyncMode S,
            GlobalCompletionEvent * GCE = &impl::local_gce,
            int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
            TaskMode B = TaskMode::Bound,
            typename T = decltype(nullptr),
            typename F = decltype(nullptr) >
  void forall(GlobalAddress<T> base, int64_t nelems, F loop_body) {
    impl::forall<B,S,GCE,Threshold>(base, nelems, loop_body, &F::operator());
  }
  
  /// Overload to allow using default GCE but specifying threshold
  template< int64_t Threshold,
            GlobalCompletionEvent * GCE = &impl::local_gce,
            TaskMode B = TaskMode::Bound,
            SyncMode S = SyncMode::Blocking,
            typename T = decltype(nullptr),
            typename F = decltype(nullptr) >
  void forall(GlobalAddress<T> base, int64_t nelems, F loop_body) {
    impl::forall<B,S,GCE,Threshold>(base, nelems, loop_body, &F::operator());
  }
  
  /// Overload for specifying GCE only
  template< GlobalCompletionEvent * GCE,
            int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
            TaskMode B = TaskMode::Bound,
            SyncMode S = SyncMode::Blocking,
            typename T = decltype(nullptr),
            typename F = decltype(nullptr) >
  void forall(GlobalAddress<T> base, int64_t nelems, F loop_body) {
    impl::forall<B,S,GCE,Threshold>(base, nelems, loop_body, &F::operator());
  }
  
  /// @}
  
} // namespace Grappa

#endif
