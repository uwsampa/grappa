#ifndef __GLOBALMEMORY_HPP__
#define __GLOBALMEMORY_HPP__

#include <stdint.h>

#include "CoreBuffer.hpp"

typedef struct mailbox {
    bool flag;
    uint64_t data;
} Descriptor;

class GlobalMemory {
    private:
        CoreBuffer* to;
        CoreBuffer* from;

// TODO hashmap impl
// TODO thread id?
        HashMap<int,Descriptor*> mailboxes;

        Descriptor* getDescriptor();
        void releaseDescriptor(Descriptor*);
    public:
        GlobalMemory(CoreBuffer* req_q, CoreBuffer* resp_q) 
            : to  (req_q)
            , from (resp_q) 
            , mailboxes (new HashMap<uint64_t,Descriptor*>()) {}
            

        void readIssue(uint64_t gaddr, int local_tid);

        uint64_t readComplete(uint64_t gaddr);


};

void GlobalMemory::readIssue(uint64_t gaddr, int local_tid) {
   Descriptor* mailbox = getDescriptor();

   mailboxes.put(local_tid, mailbox); 
   
   // TODO make queues configurable to larger values
   // XXX for now just send a mailbox address
   //uint64_t request = (gaddr<<32) | local_tid; 
   uint64_t request = (uint64_t) mailbox;

   
   to->produce(request);
}

uint64_t GlobalMemory::readComplete(uint64_t gaddr, thread* me) {
    
    Descriptor* m = mailboxes.remove(me->id);
    while (true) { 
        if (m->flag) {   // XXX synchro/volatile?
            uint64_t resp = m->data;
            return resp;
        } else {
            thread_yield(me);
        }
    }
    
    /*while(true) {
        while (from->canConsume()) {
            uint64_t response = from->consume();
            uint64_t resp_addr = (response>>32) & 0x00000000ffffffff;
            int resp_tid = response & 0x00000000ffffffff;
//...*/
}

Descriptor* GlobalMemory::getDescriptor() {
    // TODO have pool of descriptors to avoid malloc
    // allocate them in efficient way (all together?/batched?/different cachelines?)

    return (Descriptor*) malloc (sizeof(Descriptor));
}
 
void GlobalMemory::releaseDescriptor(Descriptor* desc) {
    // TODO just put back in pool

    free(desc);
}


#endif
