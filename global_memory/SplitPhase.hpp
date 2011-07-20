#ifndef __SPLITPHASE_HPP__
#define __SPLITPHASE_HPP__

#include <stdint.h>
#include <tr1/unordered_map>

#include "GmTypes.hpp"
#include "CoreQueue.hpp"
#include "thread.h"
#include "MemoryDescriptor.hpp"
#include "GlobalMemory.hpp"

#include "ga++.h"

#include "timing.h"

typedef threadid_t mem_tag_t;

class SplitPhase {
    private:
        CoreQueue<uint64_t>* to;
        CoreQueue<uint64_t>* from;

        int num_clients;
        int num_waiting_unflushed;
        int64_t* local_array;
        int64_t local_begin;
        int64_t local_end;

        bool _flushIfNeed(thread* me);
        bool _isLocal(int64_t index);
 
        MemoryDescriptor* descriptors;

        void releaseDescriptor(MemoryDescriptor*);

    public:

        Timer* timer;

        MemoryDescriptor* getDescriptor(threadid_t tid);
        uint64_t local_req_count;
        uint64_t remote_req_count;

        SplitPhase(CoreQueue<uint64_t>* req_q, CoreQueue<uint64_t>* resp_q, int num_clients, int64_t* local_array, int64_t local_begin, int64_t local_end) 
            : to  (req_q)
            , from (resp_q) 
            , num_clients(num_clients)
            , num_waiting_unflushed(0) 
            , local_array(local_array)
            , local_begin(local_begin)
            , local_end(local_end)
            , descriptors (new MemoryDescriptor[num_clients+1])            
            , timer(timing_createTimer(33, 1600))
            , local_req_count(0),remote_req_count(0) 
            {
                printf("proc%d local_begin:%ld, local_end:%ld\n", GA::nodeid(), local_begin, local_end);
            
            }
            
        ~SplitPhase() {
            free(descriptors);
        }

        mem_tag_t issue(oper_enum operation, int64_t index, uint64_t data, thread* me);

        int64_t complete( mem_tag_t , thread* me);

        void unregister(thread* me);
};


#endif
