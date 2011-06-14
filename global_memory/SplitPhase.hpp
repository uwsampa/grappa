#ifndef __SPLITPHASE_HPP__
#define __SPLITPHASE_HPP__

#include <stdint.h>
#include <tr1/unordered_map>
#include <assert.h>

#include "GmTypes.hpp"
#include "CoreQueue.hpp"
#include "thread.h"
#include "MemoryDescriptor.hpp"

typedef threadid_t mem_tag_t;

class SplitPhase {
    private:
        CoreQueue<uint64_t>* to;
        CoreQueue<uint64_t>* from;
 
        // TODO for now one static descriptor per coro
        typedef std::tr1::unordered_map<const threadid_t, MemoryDescriptor*, std::tr1::hash<uint64_t>, std::equal_to<uint64_t> > DMap_t;
        DMap_t* descriptors;

        MemoryDescriptor* getDescriptor(threadid_t tid);
        void releaseDescriptor(MemoryDescriptor*);
    public:
        SplitPhase(CoreQueue<uint64_t>* req_q, CoreQueue<uint64_t>* resp_q) 
            : to  (req_q)
            , from (resp_q) 
            , descriptors (new DMap_t()) {}
            

        mem_tag_t issue(oper_enum operation, uint64_t* addr, uint64_t data, thread* me);

        uint64_t complete( mem_tag_t , thread* me);
};

mem_tag_t SplitPhase::issue(oper_enum operation, uint64_t* addr, uint64_t data, thread* me) {
   // create a tag to identify this request
   // TODO: right now just the threadid, since one outstanding request allowed
   mem_tag_t ticket = (mem_tag_t) me->id;
  
   MemoryDescriptor* desc = getDescriptor(me->id);
   /*not sure where to do this*/desc->setEmpty();
   desc->setOperation(operation);
   desc->setAddress(addr);
   desc->setData(data); // TODO for writes is full start set or does full mean done, etc..?
  
   
   // TODO configure queues to larger values
   // XXX for now just send a mailbox address
   uint64_t request = (uint64_t) desc;

   // send request to delegate
   while (!to->tryProduce(request)) {
       thread_yield(me);
   }

   // TODO for now flush always
   // (how can optimize without causing deadlock? One way might be scheduler can do flush if all
   // coroutines are waiting)
   to->flush();

   return ticket;
}

uint64_t SplitPhase::complete(mem_tag_t ticket, thread* me) {
    threadid_t tid = (threadid_t) ticket; 
    MemoryDescriptor* mydesc = (*descriptors)[tid];
int debugi=0;
    while(true) {
        printf("%d iters of waiting\n", debugi++);
        // dequeue as much as possible
        uint64_t dat;
        MemoryDescriptor* m;
        while (from->tryConsume(&dat)) {
            m = (MemoryDescriptor*) dat;
            //if (!m->isFull()) { printf("memory desc %lx gets isFull()=%c\n", m, m->isFull()); }
            assert(m->isFull()); // TODO decide what condition indicates write completion
        }
        
        // check for my data
        if (mydesc->isFull()) { // TODO decide what condition indicates write completion
            uint64_t resp = m->getData();
            releaseDescriptor(m);
            return resp;
        }
        thread_yield(me);
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


#endif
