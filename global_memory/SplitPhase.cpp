
#include <assert.h>
#include <omp.h>
#include <stdio.h>

#include "SplitPhase.hpp"
#include "GmTypes.hpp"
#include "CoreQueue.hpp"
#include "thread.h"
#include "MemoryDescriptor.hpp"

#define LOCALITY 0
#define prefetch(addr) __builtin_prefetch((addr),0,LOCALITY)

#define PREFETCH_LOCAL 1

bool SplitPhase::_isLocal(int64_t index) {
    return (local_begin <= index) && (index < local_end);
}

mem_tag_t SplitPhase::issue(oper_enum operation, int64_t index, int64_t data, thread* me) {
   // create a tag to identify this request
   // TODO: right now just the threadid, since one outstanding request allowed
   mem_tag_t ticket = (mem_tag_t) me->id;

   MemoryDescriptor* desc = getDescriptor(me->id);
   /*not sure where to do this*/desc->setEmpty();
   desc->setOperation(operation);
   desc->setAddress(index);
   desc->setData(data); // TODO for writes is full start set or does full mean done, etc..?

    // if local non synchro/quit then handle directly
    // TODO prefetch?
   if (operation==READ && _isLocal(index)) {
       #if PREFETCH_LOCAL
        prefetch(&local_array[index]);
       #endif
       return ticket;
   }


   // TODO configure queues to larger values
   // XXX for now just send a mailbox address
   uint64_t request = (uint64_t) desc;

   // send request to delegate
   while (!to->tryProduce(request)) {
       thread_yield(me);
   }

   //printf("core%u-thread%u: issued descriptor(%lx) addr=%lx, full=%d\n", omp_get_thread_num(), me->id, (uint64_t) desc, (uint64_t)desc->getAddress(), desc->isFull());

   // using num_waiting_unflushed to optimize flushes
   if (operation==QUIT) to->flush();

   return ticket;
}

void SplitPhase::_flushIfNeed() {
    if (num_waiting_unflushed>=num_clients) {
        to->flush();
        num_waiting_unflushed = 0;
    }
}


int64_t SplitPhase::complete(mem_tag_t ticket, thread* me) {
    threadid_t tid = (threadid_t) ticket;
    MemoryDescriptor* mydesc = (*descriptors)[tid];
     
     
    num_waiting_unflushed++;
    _flushIfNeed();

    int64_t index = my_desc->getAddress();
    if (mydesc->getOperation()==READ && _isLocal(index)) {
        return local_array[index];
    } else {
        while(true) {
            //printf("%d iters of waiting\n", debugi++);
            // dequeue as much as possible
            int64_t dat;
            MemoryDescriptor* m;
          /*  while (from->tryConsume(&dat)) {
                m = (MemoryDescriptor*) dat;
                //sleep(1);
                if (!m->isFull()) { printf("assertFAIL: descriptor(%lx)(data=%lu) gets isFull()=%d\n", (uint64_t)m, m->getData(), m->isFull()); exit(1); }
                //assert(m->isFull()); // TODO decide what condition indicates write completion
            }*/

            // check for my data
            if (mydesc->isFull()) { // TODO decide what condition indicates write completion
                int64_t resp = mydesc->getData();
                releaseDescriptor(mydesc);

                
                //printf("core%u-thread%u: completed descriptor(%lx) addr=%lx, data=%lx, full=%d\n", omp_get_thread_num(), me->id, (uint64_t) mydesc, (uint64_t)mydesc->getAddress(), resp, mydesc->isFull());
                return resp;
            }
            //printf("core%u-thread%u: no luck yet descriptor(%lx) addr=%lx\n", omp_get_thread_num(), me->id, (uint64_t) mydesc, (uint64_t)mydesc->getAddress());
            thread_yield(me);
        }
    }
}

MemoryDescriptor* SplitPhase::getDescriptor(threadid_t thread_id) {
    MemoryDescriptor* d = (*descriptors)[thread_id];
    if (!d) {
        d = new MemoryDescriptor();
        d->setThreadId(thread_id);
        (*descriptors)[thread_id] = d;
    }
    return d;
}

void SplitPhase::releaseDescriptor( MemoryDescriptor* descriptor) {
    // noop
}

void SplitPhase::unregister(thread* me) {
    num_clients--;
    _flushIfNeed();
}
