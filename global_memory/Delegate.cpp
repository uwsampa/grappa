#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "Delegate.hpp"
#include "thread.h"
#include "MemoryDescriptor.hpp"
#include "GmTypes.hpp"

#include "BufferedPrinter.hpp"


/*****DEBUG*************************************/
#define IS_DEBUG 0
#define IS_INFO 0 

#if IS_DEBUG
	#define DEBUGP debug_print
#else
	#define DEBUGP debug_noop
#endif

#if IS_INFO
    #define INFOP debug_print
#else
    #define INFOP debug_noop
#endif

void debug_print(void* obj, const char* formatstr, ...) {
	const char* prefix = "DELEGATE(%lx) %s";
	size_t total = strlen(prefix) + strlen(formatstr) + 32;
	char debugstr[total];
	sprintf(debugstr, prefix, (uint64_t)obj, formatstr);

	va_list al;
	va_start(al, formatstr);
	vprintf(debugstr, al);
//	vbprintf(debugstr, al);
	va_end(al);
}

void debug_noop(void* o, const char* c, ...) { return; }
/**********************************************/

#define rdtscll(val) do { \
  unsigned int __a,__d; \
  asm volatile("rdtsc" : "=a" (__a), "=d" (__d)); \
  (val) = ((unsigned long)__a) | (((unsigned long)__d)<<32); \
  } while(0)



Delegate::Delegate(CoreQueue<uint64_t>** qs_out, CoreQueue<uint64_t>** qs_in, uint32_t numLoc, GlobalMemory* gm) 
    : inQs(qs_in)
    , outQs(qs_out)
    , numLocal(numLoc)
    , global_mem(gm)
	, activeCount(numLoc) {}
    

// arguments for coroutine runnables
typedef struct hrr_thread_args {
    GlobalMemory* gm;
    Delegate* del;
} hrr_thread_args;

typedef struct hsr_thread_args {
    CoreQueue<uint64_t>** oqs;
    GlobalMemory* gm;
    Delegate* del;
} hsr_thread_args;

typedef struct hlr_thread_args {
    CoreQueue<uint64_t>* iq;
    CoreQueue<uint64_t>* oq;
    GlobalMemory* gm;
    Delegate* del;
    int queue_num;
} hlr_thread_args;


// runnable to handle the responses for local requests to remote memory
// TODO: may not be needed if return path excludes delegate
void handle_responses(thread* me, void* args) {
    hsr_thread_args* cargs = (hsr_thread_args*) args;

    CoreQueue<uint64_t>** oqs = cargs->oqs;
    GlobalMemory* gm = cargs->gm;

    Delegate* del = cargs->del;

    while (del->doContinue()) {
        //printf("Delegate: iteration of handle_responses %d\n",del->activeCount);
        MemoryDescriptor* r_resp = NULL;
        r_resp = gm->getRemoteResponse();
        if (r_resp) {
            //DEBUGP(del, "got RemoteResponse descriptor(%lx)\n", (uint64_t)r_resp);

            // glob_mem does this commented stuff
            /*uint64_t data = r_resp.data;
            uint64_t id = r_resp.id;
            MemoryDescriptor* md = gm->getDescriptorFromId(id);
            md->fillData(data);*/

          //  CoreQueue<uint64_t>* oq = oqs[r_resp->getCoreId()];
            /*while(!oq->tryProduce((uint64_t)r_resp)) {} // TODO this could block progress other nonfull queues          
            oq->flush(); // TODO for now always flush, optimize later*/

            //XXX this print stement can be after descriptor is recycled 
            //DEBUGP(del, "produced descriptor(%lx)(data=%lx,full=%d,addr=%lx) to core %u\n", (uint64_t)r_resp, r_resp->getData(), r_resp->full, r_resp->getAddress(), r_resp->getCoreId());
        }
        thread_yield(me);
    }
}


// runnable to process remote nodes' requests to our local memory
void handle_remote_requests(thread* me, void* args) {
    hrr_thread_args* cargs = (hrr_thread_args*) args;
    GlobalMemory* gm = cargs->gm;

    Delegate* del = cargs->del;

    while (del->doContinue()) {
        //DEBUGP(del, "iteration of handle_remote_requests %d\n",del->activeCount);
        
        // check if there is a remote request to process
        request_node_t r_msg;
        bool r_msg_valid = gm->getRemoteRequest(&r_msg); //TODO grab as much as possible (e.g. mpi testsome?)
        if (r_msg_valid) {
            // unpack message
            nodeid_t r_fromid = r_msg.node_id;
            MemoryDescriptor* r_req = r_msg.request;
            uint64_t* addr = r_req->getAddress();
            oper_enum operation = r_req->getOperation();
            uint64_t data = r_req->getData();

            //DEBUGP(del, "got RemoteRequest from %u; addr=%lx,op=%d,data=%lu,descriptor(%lx)\n", r_fromid, (uint64_t)addr,operation,data,(uint64_t)r_req);

            // perform the requested operation
            switch(operation) {
                case READ: {
                   uint64_t resultdata = *addr;     // do the local memory access
                   MemoryDescriptor* resp = r_req;  //TODO: reference copy as done here only works for shmem
                   resp->fillData(resultdata);

                  /* unneeded while fill result in shmem domain already*/
                  // while (!gm->sendResponse(r_fromid, resp)) {
                  //     thread_yield(me);
                  // }
                   
                   DEBUGP(del, "sent response descriptor(%lx) to %u; addr=%lx; data=%lx; full=%d\n", (uint64_t)resp, r_fromid, addr, resp->getData(), resp->isFull());
                   break;
                }
                default:
                    assert(false);

            }
        }

        thread_yield(me);
    }
}


// runnable for processing global memory requests from local cores
void handle_local_requests(thread* me, void* args) {
    hlr_thread_args* cargs = (hlr_thread_args*) args;

    CoreQueue<uint64_t>* iq = cargs->iq;
    CoreQueue<uint64_t>* oq = cargs->oq;
    GlobalMemory* gm = cargs->gm;
    Delegate* del = cargs->del;
    int queue_num = cargs->queue_num;

   /* ime flush
    const unsigned long num_ticks_flush = 5400;
    unsigned long nextFlush;
    rdtscll(nextFlush);
    nextFlush += num_ticks_flush;
*/

    while (del->doContinue()) {
        DEBUGP(del, "iteration of handle_local_requests %d\n", del->activeCount);

        // check for a request
        uint64_t data;
        bool none_deq = true;
        while (iq->tryConsume(&data)) {
            none_deq=false;
            MemoryDescriptor* md = (MemoryDescriptor*) data;
           // md->setCoreId(queue_num); // keep track of where it came from

            DEBUGP(del, "got local request descriptor(%lx)\n", (uint64_t)md);
            bool isLocal = gm->isLocal(md);

            switch (md->getOperation()) {
                case READ: {
                    if (isLocal) {
                        uint64_t* addr = gm->getLocalAddress(md);
                        // DEBUGP(del, "got read to local; addr=%lx\n", (uint64_t)addr);
                        // TODO pref/yield?
                        // and perhaps want a blocking produce for coros
                        uint64_t result = *addr;
                        md->fillData(result);

                        /* for now no return queue used    
                        while(!oq->tryProduce((uint64_t)md)) {
                            thread_yield(me);
                        }
                        oq->flush(); // TODO for now always flush, optimize later*/
                       
//                        DEBUGP(del, "produced descriptor(%lx) to core %u; data=%lx\n", (uint64_t)md, md->getCoreId(), md->getData());
                    } else {
                    	DEBUGP(del, "got read to remote; addr=%lx\n", (uint64_t)md->getAddress());
                        /* if to save bandwidth want request num bytes to be 
                        only just big enough to support the max number 
                        of outstanding requests */
                        
                        while (!gm->sendRequest(md)) {
                            thread_yield(me);
                        }
                        
                        //DEBUGP(del, "sent request for addr=%lx to remote\n", (uint64_t)md->getAddress());
                    }
                    break;
                }
                case QUIT: {
                	del->decrementActive();
                	break;
                }
                default:
                    assert(false);// for now only reads
            }
        }
     /* timed flush   
     if (none_deq) {
            unsigned long current;
            rdtscll(current);
            if (current > nextFlush) {
                gm->flushSendReq();
                nextFlush = current + num_ticks_flush;
            }
        }
         */   

        thread_yield(me); 
//TODO with this scheme can still starve others
// alternative is to iterate across queues and only work on a request if (!local || can produce). Just requires a single buffer space for each queue to hold the peeked value.
// Perhaps currently aggregatable values can be processed first
    }
}

void Delegate::decrementActive() {
	activeCount--;
	//DEBUGP(this, "decrement active to %d\n", activeCount);
}

bool Delegate::doContinue() {
	return activeCount > 0;
}

void Delegate::run() {

    thread* master = thread_init();
    scheduler* scheduler = create_scheduler(master);
   
   // create threads that handle local requests 
   hlr_thread_args hlrargs[numLocal]; 
   for (uint32_t client_id=0; client_id<numLocal; client_id++) {
       hlrargs[client_id].iq = inQs[client_id];
       hlrargs[client_id].oq = outQs[client_id];
       hlrargs[client_id].gm = global_mem;
       hlrargs[client_id].del = this;
       hlrargs[client_id].queue_num = client_id;
       thread_spawn(master, scheduler, handle_local_requests, &hlrargs[client_id]);
   }


   // create threads that handle remote requests to local memory
   // number of coros for handle local latency
   uint32_t num_hrr = 6;
   hrr_thread_args hrrargs[num_hrr];
   for (uint32_t t=0; t<num_hrr; t++) {
        hrrargs[t].gm = global_mem;
        hrrargs[t].del = this;
        thread_spawn(master, scheduler, handle_remote_requests, &hrrargs[t]);
   }
   
    
   // create threds that receive satisfied remote requests
   hsr_thread_args hsrarg;
   hsrarg.oqs = outQs;
   hsrarg.gm = global_mem;
   hsrarg.del = this;
//   thread_spawn(master, scheduler, handle_responses, &hsrarg);
  /* hlr_threads_args hsrargs[numLocal];
   for (uint32_t client_id=0; client_id<numLocal; client_id++) {
       CoreQueue<uint64_t>* iq = inQs[client_id];
       CoreQueue<uint64_t>* oq = outQs[client_id];
       hsrargs[client_id].iq = iq;
       hsrargs[client_id].oq = oq;
       hsrargs[client_id].gm = global_mem;
       thread_spawn(master, scheduler, handle_responses, &hsrargs[client_id]);
   }*/

   //DEBUGP(this, "starting its tasks\n");

    // start
    run_all(scheduler);
}
 
