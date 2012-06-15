#ifndef __PARALLEL_LOOP_HPP__
#define __PARALLEL_LOOP_HPP__

DEFINE_int64( parallel_loop_threshold, 1, "threshold for how small a group of iterations should be to perform them serially" );

// implementations
#include "ParallelLoop_future.hpp"
//#include "ParallelLoop_single.hpp"
#include "parallel_loop_impls/ParallelLoop_singleSerial.hpp"

#include "parallel_loop_impls/ParallelLoop_semaphore.hpp"

#endif
