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

DEFINE_int64( loop_threshold, 1, "threshold for how small a group of iterations should be to perform them serially" );

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
    enum class SpawnType: short {
      PRIVATE,
      PUBLIC
    };
    
    template< int64_t Threshold, SpawnType ST, typename F>
    void loop_decomposition(int64_t start, int64_t iterations, F loop_body) {
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
        switch(ST) {
          case SpawnType::PRIVATE:
            privateTask([rstart, riters, loop_body] {
              loop_decomposition<Threshold,ST,F>(rstart, riters, loop_body);
            }); break;
          case SpawnType::PUBLIC:
            publicTask([rstart, riters, loop_body] {
              loop_decomposition<Threshold,ST,F>(rstart, riters, loop_body);
            }); break;
        }
        
        // left side here
        loop_decomposition<Threshold,ST,F>(start, (iterations+1)/2, loop_body);
      }
    }
    
    template<SpawnType ST, typename F>
    inline void loop_decomposition(int64_t start, int64_t iters, F loop_body) {
      loop_decomposition<STATIC_LOOP_THRESHOLD,ST,F>(start, iters, loop_body);
    }
    
    template<int64_t Threshold, typename F>
    inline void loop_decomposition(int64_t start, int64_t iters, F loop_body) {
      loop_decomposition<Threshold,SpawnType::PRIVATE,F>(start, iters, loop_body);
    }

  }
  
} // namespace Grappa

#endif
