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
    
    template< int64_t Threshold, typename F >
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
        privateTask([start, iterations, loop_body] {
          loop_decomposition<Threshold,F>(start+(iterations+1)/2, iterations/2, loop_body);
        });
        
        // left side here
        loop_decomposition<Threshold,F>(start, (iterations+1)/2, loop_body);
      }
    }
    
    template<typename F>
    inline void loop_decomposition(int64_t start, int64_t iters, F loop_body) {
      loop_decomposition<STATIC_LOOP_THRESHOLD,F>(start, iters, loop_body);
    }
  }
  
} // namespace Grappa

#endif
