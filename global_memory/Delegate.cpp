#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "Delegate.hpp"
#include "thread.h"
#include "MemoryDescriptor.hpp"
#include "GmTypes.hpp"



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



Delegate::Delegate(CoreQueue<uint64_t>** qs_out, CoreQueue<uint64_t>** qs_in, uint32_t numLoc, GA::GlobalArray* vertices) 
    : inQs(qs_in)
    , outQs(qs_out)
    , numLocal(numLoc)
    , vertices(vertices)
    , handles()
	, activeCount(numLoc) {}
    


// arguments for coroutine runnables
typedef struct hsr_thread_args {
    Delegate* del;
} hsr_thread_args;

typedef struct hlr_thread_args {
    CoreQueue<uint64_t>* iq;
    CoreQueue<uint64_t>* oq;
    Delegate* del;
    int queue_num;
} hlr_thread_args;


// runnable to handle the responses for local requests to remote memory
// TODO: may not be needed if return path excludes delegate
void handle_responses(thread* me, void* args) {
    hsr_thread_args* cargs = (hsr_thread_args*) args;

    Delegate* del = cargs->del;
    hlist_t handles = del->handles;

    while (del->doContinue()) {
        //printf("Delegate: iteration of handle_responses %d\n",del->activeCount);

        hlist_t::iterator iter;
        for (iter = handles.begin(); iter!=handles.end(); iter++) {
            request_handle_t rh = *iter;
            if (1 /* TODO armci.test first */) {
                GA::nbWait( &(rh.handle) );
                rh.md->setFull(); // TODO (only for Read)
                handles.pop_front();
            }
        }
        
            //DEBUGP(del, "got RemoteResponse descriptor(%lx)\n", (uint64_t)r_resp);

            // glob_mem does this commented stuff
            /*uint64_t data = r_resp.data;
            uint64_t id = r_resp.id;
            MemoryDescriptor* md = gm->getDescriptorFromId(id);
            md->fillData(data);*/

            /*while(!oq->tryProduce((uint64_t)r_resp)) {} // TODO this could block progress other nonfull queues          
            oq->flush(); // TODO for now always flush, optimize later*/

            //XXX this print stement can be after descriptor is recycled 
            //DEBUGP(del, "produced descriptor(%lx)(data=%lx,full=%d,addr=%lx) to core %u\n", (uint64_t)r_resp, r_resp->getData(), r_resp->full, r_resp->getAddress(), r_resp->getCoreId());
        thread_yield(me);
    }
}



// runnable for processing global memory requests from local cores
void handle_local_requests(thread* me, void* args) {
    hlr_thread_args* cargs = (hlr_thread_args*) args;

    CoreQueue<uint64_t>* iq = cargs->iq;
    CoreQueue<uint64_t>* oq = cargs->oq;
    Delegate* del = cargs->del;
    GA::GlobalArray* vertices = del->vertices;
    hlist_t handles = del->handles;
    int queue_num = cargs->queue_num;


    while (del->doContinue()) {
        DEBUGP(del, "iteration of handle_local_requests %d\n", del->activeCount);

        // check for a request
        uint64_t data;
        while (iq->tryConsume(&data)) {
            MemoryDescriptor* md = (MemoryDescriptor*) data;
            md->setCoreId(queue_num); // keep track of where it came from

            DEBUGP(del, "got local request descriptor(%lx)\n", (uint64_t)md);

            switch (md->getOperation()) {
                case READ: {
                    int64_t index = md->getAddress();

                    DEBUGP(del, "got read to remote; addr=%lx\n", (uint64_t)md->getAddress());
                    /* if to save bandwidth want request num bytes to be 
                    only just big enough to support the max number 
                    of outstanding requests */
                  
                    // post a get request 
                    request_handle_t rh;
                    rh.md = md;
                    vertices->nbGet( &index, &index, md->getDataFieldAddress(), NULL, &(rh.handle));
                    handles.push_back(rh);
                    
                    //DEBUGP(del, "sent request for addr=%lx to remote\n", (uint64_t)md->getAddress());
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
       hlrargs[client_id].del = this;
       hlrargs[client_id].queue_num = client_id;
       thread_spawn(master, scheduler, handle_local_requests, &hlrargs[client_id]);
   }

    
   // create threads that receive satisfied remote requests
   hsr_thread_args hsrarg;
   hsrarg.del = this;
   thread_spawn(master, scheduler, handle_responses, &hsrarg);
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
 
