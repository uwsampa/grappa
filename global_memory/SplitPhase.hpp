#ifndef __SPLITPHASE_HPP__
#define __SPLITPHASE_HPP__

#include <stdint.h>
#include <tr1/unordered_map>

#include "GmTypes.hpp"
#include "CoreQueue.hpp"
#include "thread.h"
#include "MemoryDescriptor.hpp"
#include "GlobalMemory.hpp"

typedef threadid_t mem_tag_t;

class SplitPhase {
    private:
        CoreQueue<uint64_t>* to;
        CoreQueue<uint64_t>* from;

        int num_clients;
        int num_waiting_unflushed;
        void _flushIfNeed();
 
        // TODO for now one static descriptor per coro
        typedef std::tr1::unordered_map<const threadid_t, MemoryDescriptor*, std::tr1::hash<uint64_t>, std::equal_to<uint64_t> > DMap_t;
        DMap_t* descriptors;

        GlobalMemory* gm;

        MemoryDescriptor* getDescriptor(threadid_t tid);
        void releaseDescriptor(MemoryDescriptor*);

    public:
        SplitPhase(CoreQueue<uint64_t>* req_q, CoreQueue<uint64_t>* resp_q, int num_clients, GlobalMemory* gm) 
            : to  (req_q)
            , from (resp_q) 
            , descriptors (new DMap_t()) 
            , num_clients(num_clients)
            , num_waiting_unflushed(0) 
            , gm(gm) {}
            
        ~SplitPhase() {
        	delete descriptors;
        }

        mem_tag_t issue(oper_enum operation, uint64_t* addr, uint64_t data, thread* me);

        uint64_t complete( mem_tag_t , thread* me);

        void unregister(thread* me);
};


#endif
