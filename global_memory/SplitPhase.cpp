
#include <assert.h>
#include <omp.h>
#include <stdio.h>

#include "SplitPhase.hpp"
#include "GmTypes.hpp"
#include "CoreQueue.hpp"
#include "thread.h"
#include "MemoryDescriptor.hpp"

#include "ga++.h"


bool SplitPhase::_flushIfNeed(thread* me) {
    if (num_waiting_unflushed>=num_clients) {
        to->flush();
  //      printf("proc%d-core%u: forced flush\n", GA::nodeid(), omp_get_thread_num());
        num_waiting_unflushed = 0;
        #if SP_BLOCK_UNTIL_FLUSH
            threads_wake(me);
        #endif
        return true;
    } else {
        return false;
    }
}

MemoryDescriptor* SplitPhase::getDescriptor(threadid_t thread_id) {
    return descriptors + thread_id; //assumes threadid assigned contiguously starting at 0
}

void SplitPhase::releaseDescriptor( MemoryDescriptor* descriptor) {
    // noop
}

void SplitPhase::unregister(thread* me) {
    num_clients--;
    _flushIfNeed(me);
}
