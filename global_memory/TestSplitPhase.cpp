#include <stdio.h>

#include "SplitPhase.hpp"
#include "GmTypes.hpp"
#include "MemoryDescriptor.hpp"
#include "thread.h"

typedef struct run_thr_args {
    SplitPhase* mem;
    uint64_t result;
} run_thr_args;

void run_thr_target(thread* me, void* args) {
    run_thr_args* cargs = (run_thr_args*) args;

    SplitPhase* sp = cargs->mem;

 
    uint64_t addr = 0xBEEF;
    mem_tag_t tag = sp->issue(READ, (uint64_t*)addr, 0, me);
    printf("thread %lx; c issued\n", (uint64_t) me);

    // switch
    thread_yield(me);
    printf("c woke\n");

    cargs->result = sp->complete(tag,me);
    printf("c completed\n");

    //done
    thread_exit(me, NULL);
}

// thread to play delegate side
typedef struct stub_thr_args {
    CoreQueue<uint64_t>* toMe;
    CoreQueue<uint64_t>* fromMe;
    SplitPhase* sp;
    uint64_t intresult;
} stub_thr_args;


void stub_thr_target (thread* me, void* args) {
    stub_thr_args* cargs = (stub_thr_args*) args;

//    printf("thread %lx; d starts trying to consume\n",(uint64_t)me);
    uint64_t data;
    while(!cargs->toMe->tryConsume(&data)) { }
//    printf("d consumed %lu\n", data);

    MemoryDescriptor* desc = (MemoryDescriptor*) data;
    uint64_t intdata = (uint64_t)  desc->getAddress();
    cargs->intresult = intdata;
    desc->fillData(intdata+1);

//    printf("d starts trying to produce\n");
    while(!cargs->fromMe->tryProduce(data)) { }    
    cargs->fromMe->flush();
    printf("d produced and flushed\n");
    
    thread_exit(me, NULL);
}

void nothing_target(thread* me, void* args) {
    printf("Nothing: hi!\n");
    thread_exit(me, NULL);
}

int main() {
    CoreQueue<uint64_t>* toDel = CoreQueue<uint64_t>::createQueue();
    CoreQueue<uint64_t>* fromDel = CoreQueue<uint64_t>::createQueue();
    SplitPhase* sp = new SplitPhase(toDel, fromDel);

    thread* master = thread_init();
    scheduler* s = create_scheduler(master);

    run_thr_args r_arg;
    r_arg.mem = sp;
    r_arg.result = 0;

    thread* rt = thread_spawn(master, s, run_thr_target, &r_arg);

//////
thread* harmless = thread_spawn(master, s, nothing_target, &r_arg);



    stub_thr_args st_arg;
    st_arg.toMe = toDel;
    st_arg.fromMe = fromDel;
    st_arg.sp = sp;
    
    thread* stt = thread_spawn(master, s, stub_thr_target, &st_arg);

    run_all(s);
 
    printf("intresult:%lu\n", st_arg.intresult);
    printf("result:%lu\n", r_arg.result);   
}

