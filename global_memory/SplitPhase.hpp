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

#include "timing.hpp"

//gasnet and global array
#include <gasnet.h>
#include "global_memory.h"
#include "global_array.h"

#define SP_LOCALITY 0
#define prefetch(addr) __builtin_prefetch((addr),0,SP_LOCALITY)
#define SP_PREFETCH_LOCAL 1
#define SP_BLOCK_UNTIL_FLUSH 0
#define SP_TIME_ENQUEUES 0
#define SP_TIME_REQ_LATENCY 0

typedef struct mem_tag_t {
    void* addr;
    gasnet_handle_t nbhandle;
    bool handle_locally;
} mem_tag_t;

class SplitPhase {
    private:
        CoreQueue<uint64_t>* to;
        CoreQueue<uint64_t>* from;

        int num_clients;
        int num_active_clients;
        int num_waiting_unflushed;

        bool _flushIfNeed(thread* me);
        bool _isLocal(int64_t index);
 
        MemoryDescriptor* descriptors;

        void releaseDescriptor(MemoryDescriptor*);

    public:

        Timer* timer;

        MemoryDescriptor* getDescriptor(threadid_t tid);
        uint64_t local_req_count;
        uint64_t remote_req_count;

        SplitPhase(CoreQueue<uint64_t>* req_q, CoreQueue<uint64_t>* resp_q, int num_clients) 
            : to  (req_q)
            , from (resp_q) 
            , num_clients(num_clients)
            , num_active_clients(num_clients)
            , num_waiting_unflushed(0) 
            , descriptors (new MemoryDescriptor[num_clients+1])            
            , timer(Timer::createTimer("sp_enq", 33, 1600))
            , local_req_count(0),remote_req_count(0) 
            {
                printf("proc%d local_begin:%ld, local_end:%ld\n", GA::nodeid(), local_begin, local_end);
                timer->setPrint(SP_TIME_ENQUEUES); 
            }
            
        ~SplitPhase() {

            double sum_time=0;
            for (int i=1; i<num_clients+1; i++) {
                sum_time += descriptors[i].latency_timer.avgIntervalNs();
            }
            delete[] descriptors;
            printf("threads avg wait: %f\n", sum_time/num_clients);

            delete timer;
        }

        mem_tag_t issue(oper_enum operation, global_address gaddr, uint64_t data, thread* me);

        int64_t complete( mem_tag_t , thread* me);

        void unregister(thread* me);
};


inline mem_tag_t SplitPhase::issue(oper_enum operation, global_address gaddr, uint64_t data, thread* me) {

 // if local non synchro/quit then handle directly
   if (operation==READ && _isLocal(gaddr)) {
       local_req_count++;
       //printf("proc%d-core%u-thread%u: issue LOCAL descriptor(%lx) addr=%ld/x%lx, full=%d\n", GA::nodeid(), omp_get_thread_num(), me->id, (uint64_t) desc, (uint64_t)desc->getAddress(), (uint64_t)desc->getAddress(), desc->isFull());
        
       void* local_address = gm_local_gm_address_to_local_ptr(&gaddr);

       #if SP_PREFETCH_LOCAL
        prefetch(local_address);
       #endif

       mem_tag_t ticket;
       ticket.addr = local_address;
       ticket.handleLocally = true;

       return ticket;
   }


   MemoryDescriptor* desc = getDescriptor(me->id);
   
   desc->setData(data); // TODO for writes is full start set or does full mean done, etc..?
   

   remote_req_count++;

   int remote_node = gm_node_of_address(&gaddr);
   void* remote_ptr = gm_address_to_ptr(&gaddr);
   
   gasnet_handle_t hndl = gasnet_get_nb(md->getDataFieldAddress(), remote_node, remote_ptr, 8);

   #if SP_TIME_ENQUEUES
   timer->markTime(true);
   #endif

//   printf("proc%d-core%u-thread%u: issue REMOTE descriptor(%lx) addr=%ld/x%lx, full=%d; produceSize=%d\n", GA::nodeid(), omp_get_thread_num(), me->id, (uint64_t) desc, (uint64_t)desc->getAddress(), (uint64_t)desc->getAddress(), desc->isFull(), to->sizeProducer());

   // using num_waiting_unflushed to optimize flushes
   if (operation==QUIT) to->flush();

   mem_tag_t ticket;
   ticket.addr = (void*)desc;
   ticket.nbhandle = hndl;
   ticket.handleLocally = false;
   return ticket;
}


inline uint64_t SplitPhase::complete(mem_tag_t ticket, thread* me) {
    if (ticket.handleLocally) { 
        return *((uint64_t*)ticket.addr);
    } else {
        num_waiting_unflushed++;
        
        #if SP_BLOCK_UNTIL_FLUSH
        bool isFlushedOrWoken = _flushIfNeed(me);
        #else
        _flushIfNeed(me);
        #endif

        MemoryDescriptor* mydesc = (MemoryDescriptor*) ticket.addr;

        #if SP_TIME_REQ_LATENCY
        mydesc->latency_timer.markTime(false);
        #endif

        while(true) {

            mydesc->full_poll_count++;

            // check for my data
            
            if (GASNET_OK == gasnet_try_syncnb(ticket.nbhandle)) {
                
                #if SP_TIME_REQ_LATENCY
                mydesc->latency_timer.markTime(true);
                #endif
                
                uint64_t resp = mydesc->getData();
                releaseDescriptor(mydesc);

                
                //printf("core%u-thread%u: completed descriptor(%lx) addr=%lx, data=%lx, full=%d\n", omp_get_thread_num(), me->id, (uint64_t) mydesc, (uint64_t)mydesc->getAddress(), resp, mydesc->isFull());
                return resp;
            }
            //printf("core%u-thread%u: no luck yet descriptor(%lx) addr=%lx\n", omp_get_thread_num(), me->id, (uint64_t) mydesc, (uint64_t)mydesc->getAddress());
        
            #if SP_BLOCK_UNTIL_FLUSH
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

inline bool SplitPhase::_isLocal(global_address gaddr) {
    return gm_is_address_local(&gaddr);
}


inline bool SplitPhase::_flushIfNeed(thread* me) {
    if (num_waiting_unflushed>=num_active_clients) {
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

inline MemoryDescriptor* SplitPhase::getDescriptor(threadid_t thread_id) {
    return descriptors + thread_id; //assumes threadid assigned contiguously starting at 0
}

inline void SplitPhase::releaseDescriptor( MemoryDescriptor* descriptor) {
    // noop
}

#endif
