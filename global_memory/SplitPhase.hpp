#ifndef __SPLITPHASE_HPP__
#define __SPLITPHASE_HPP__

#include <stdint.h>
#include <tr1/unordered_map>

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
            
        ~SplitPhase() {
        	delete descriptors;
        }

        mem_tag_t issue(oper_enum operation, uint64_t* addr, uint64_t data, thread* me);

        uint64_t complete( mem_tag_t , thread* me);
};




#endif
