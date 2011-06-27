#include <stdio.h>
#include <omp.h>
#include <thread.h>

#include "GmTypes.hpp"
#include "SplitPhase.hpp"
#include "GlobalMemory.hpp"
#include "CoreQueue.hpp"
#include "Delegate.hpp"

typedef struct run_thr_args {
    SplitPhase* mem;
    uint64_t result;
    uint64_t result2;
} run_thr_args;

void run_thr_target(thread* me, void* args) {
    run_thr_args* cargs = (run_thr_args*) args;

    SplitPhase* sp = cargs->mem;

    // global request to "local memory"
    uint64_t mylocal1 = (0xBEEF<<4) | me->id;
 
    uint64_t* addr = &mylocal1;
    mem_tag_t tag = sp->issue(READ, addr, 0, me);
    printf("thread %lx; c issued\n", (uint64_t) me);

    // switch
    thread_yield(me);
    printf("c woke\n");

    cargs->result = sp->complete(tag,me);
    printf("c completed and got data=%lx\n", cargs->result);


    //global request to "remote memory"
    uint64_t rem_addr = 0xffff000011110000;
    mem_tag_t tag2 = sp->issue(READ, (uint64_t*)rem_addr, 0, me);

    // switch
    thread_yield(me);
    printf("c woke2\n");

    cargs->result2 = sp->complete(tag2, me);
    printf("c completed2 and got data=%lx\n", cargs->result2);

    //done
    thread_exit(me, NULL);
}



int main() {
    

    int num_threads = 2;

    nodeid_t myId = 0;
    GlobalMemory* gm = new GlobalMemory(myId);
   
    uint64_t data1 = 0xC0FF;
    uint64_t data2 = 0xFEED;
    
    // initialize some fake requests from remote nodes
    MemoryDescriptor req0;
    req0.setAddress(&data1);
    req0.setThreadId(0x0);
    req0.setCoreId(0x0);
    req0.setOperation(READ);
    req0.setData(0);
    gm->remreq.push(&req0); //0
    MemoryDescriptor req1;
    req1.setAddress(&data2);
    req1.setThreadId(0x01);
    req1.setCoreId(0x0);
    req1.setOperation(READ);
    req1.setData(0);
    gm->remreq.push(&req1); //1
     
    CoreQueue<uint64_t>* toDel = CoreQueue<uint64_t>::createQueue();
    CoreQueue<uint64_t>* fromDel = CoreQueue<uint64_t>::createQueue();
    SplitPhase* sp = new SplitPhase(toDel, fromDel);


    CoreQueue<uint64_t>* toDelQs[1] = { toDel };
    CoreQueue<uint64_t>* fromDelQs[1] = { fromDel };
    Delegate* del = new Delegate(fromDelQs, toDelQs, 1, gm);
    
    
    int num_st = 2;

    #pragma omp parallel for num_threads(num_threads)
    for (int c=0; c<2; c++) {
        if (c==0) {
            printf("worker hwthread starts\n");
            
            thread* master = thread_init();
            scheduler* s = create_scheduler(master);

            run_thr_args r_args[num_st];
            thread* threads[num_st];

            for (int t=0; t<num_st; t++) {
                r_args[t].mem = sp;
                r_args[t].result = 0;
                r_args[t].result2 = 0;

                threads[t] = thread_spawn(master, s, run_thr_target, &r_args[t]);
            }

            run_all(s);

            printf("worker hwthread ends\n");

        } else {
            printf("del hwthread starts\n");
            del->run();
        }
    }
}



