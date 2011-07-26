
#include <assert.h>
#include <omp.h>
#include <stdio.h>

#include "SplitPhase.hpp"
#include "GmTypes.hpp"
#include "CoreQueue.hpp"
#include "thread.h"
#include "MemoryDescriptor.hpp"

#include "ga++.h"



void SplitPhase::unregister(thread* me) {
    num_active_clients--;
    _flushIfNeed(me);
}
