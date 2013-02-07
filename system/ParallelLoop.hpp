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
  
  
} // namespace Grappa

#endif
