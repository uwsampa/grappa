
#include <assert.h>
#include <omp.h>
#include <stdio.h>

#include "SplitPhase.hpp"
#include "GmTypes.hpp"
#include "CoreQueue.hpp"
#include "thread.h"
#include "MemoryDescriptor.hpp"

#include "ga++.h"

#define LOCALITY 0
#define prefetch(addr) __builtin_prefetch((addr),0,LOCALITY)

#define PREFETCH_LOCAL 1

#define BLOCK_UNTIL_FLUSH 0

bool SplitPhase::_isLocal(int64_t index) {
    bool r = (local_begin <= index) && (index < local_end); 
    //printf("index=%ld; nodes=%d; this is local %d\n", index, GA::nodeid(), r);
    return r;
}

mem_tag_t SplitPhase::issue(oper_enum operation, int64_t index, uint64_t data, thread* me) {

 // if local non synchro/quit then handle directly
   if (operation==READ && _isLocal(index)) {
       local_req_count++;
       //printf("proc%d-core%u-thread%u: issue LOCAL descriptor(%lx) addr=%ld/x%lx, full=%d\n", GA::nodeid(), omp_get_thread_num(), me->id, (uint64_t) desc, (uint64_t)desc->getAddress(), (uint64_t)desc->getAddress(), desc->isFull());
        
       #if PREFETCH_LOCAL
        int64_t local_index = index - local_begin;
        prefetch(&local_array[local_index]);
       #endif

       mem_tag_t ticket;
       ticket.addr = (void*)local_index;
       ticket.handleLocally = true;

       return ticket;
   }


   MemoryDescriptor* desc = getDescriptor(me->id);
   /*not sure where to do this*/desc->setEmpty();
   desc->setOperation(operation);
   desc->setAddress(index);
   desc->setData(data); // TODO for writes is full start set or does full mean done, etc..?
   

   remote_req_count++;


   // TODO configure queues to larger values
   // XXX for now just send a mailbox address
   uint64_t request = (uint64_t) desc;

   // send request to delegate
   while (!to->tryProduce(request)) {
       thread_yield(me);
   }

//   printf("proc%d-core%u-thread%u: issue REMOTE descriptor(%lx) addr=%ld/x%lx, full=%d; produceSize=%d\n", GA::nodeid(), omp_get_thread_num(), me->id, (uint64_t) desc, (uint64_t)desc->getAddress(), (uint64_t)desc->getAddress(), desc->isFull(), to->sizeProducer());

   // using num_waiting_unflushed to optimize flushes
   if (operation==QUIT) to->flush();

   mem_tag_t ticket;
   ticket.addr = (void*)desc;
   ticket.handleLocally = false;
   return ticket;
}

bool SplitPhase::_flushIfNeed(thread* me) {
    if (num_waiting_unflushed>=num_clients) {
        to->flush();
  //      printf("proc%d-core%u: forced flush\n", GA::nodeid(), omp_get_thread_num());
        num_waiting_unflushed = 0;
        #if BLOCK_UNTIL_FLUSH
            threads_wake(me);
        #endif
        return true;
    } else {
        return false;
    }
}


int64_t SplitPhase::complete(mem_tag_t ticket, thread* me) {
    if (ticket.handleLocally) { 
        int64_t local_index = (int64_t)ticket.addr;
        return local_array[local_index];
    } else {
        num_waiting_unflushed++;
        bool isFlushedOrWoken = _flushIfNeed(me);

        while(true) {
            MemoryDescriptor* mydesc = (MemoryDescriptor*) ticket.addr;

            mydesc->full_poll_count++;

            // check for my data
            if (mydesc->isFull()) { // TODO decide what condition indicates write completion
                int64_t resp = mydesc->getData();
                releaseDescriptor(mydesc);

                
                //printf("core%u-thread%u: completed descriptor(%lx) addr=%lx, data=%lx, full=%d\n", omp_get_thread_num(), me->id, (uint64_t) mydesc, (uint64_t)mydesc->getAddress(), resp, mydesc->isFull());
                return resp;
            }
            //printf("core%u-thread%u: no luck yet descriptor(%lx) addr=%lx\n", omp_get_thread_num(), me->id, (uint64_t) mydesc, (uint64_t)mydesc->getAddress());
        
            #if BLOCK_UNTIL_FLUSH
            if (isFlushedOrWoken) {
                thread_yield(me);
            } else {
                thread_yield_wait(me);
                isFlushedOrWoken = true;
            }
            #else
            thread_yield(me);
            #endif
        }
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
