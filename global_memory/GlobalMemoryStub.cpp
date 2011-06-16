#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "GlobalMemory.hpp"
#include "MemoryDescriptor.hpp"

uint64_t next_fauxdata = 0xDEADBEE0;

MemoryDescriptor* GlobalMemory::getRemoteResponse() {
   int avail = rand() % 2;
   if (avail && locreq.size() > 0) {
       MemoryDescriptor* md = locreq.front();
       locreq.pop();
       md->fillData(next_fauxdata++);
       return md;
   } else {
       return NULL;
   }
}
       
void GlobalMemory::sendResponse(nodeid_t to, MemoryDescriptor* resp) {
    // blah
    printf("DEBUG: response <%u,%lu> sent\n to host %u\n", resp->getCoreId(), resp->getData(), to); 
}

bool GlobalMemory::getRemoteRequest(request_node_t* rh) {
    int avail = rand() % 2;
    if (avail && remreq.size() > 0) {
        rh->request = remreq.front();
        rh->node_id = 1; // single remote node
        remreq.pop();
        return true;
    } else {
        return false;
    }
}

void GlobalMemory::sendRequest(MemoryDescriptor* md) {
    locreq.push(md);
}



uint64_t* GlobalMemory::getLocalAddress(MemoryDescriptor* md) {
    return md->getAddress();
}

bool GlobalMemory::isLocal(MemoryDescriptor* md) {
    return nodeid==getNodeForDescriptor(md);
}
 
 
nodeid_t GlobalMemory::getNodeForDescriptor(MemoryDescriptor* md) {
    uint64_t addr = (uint64_t)md->getAddress();
    
    /* lookup addr */
    if ((addr>>48)&0xffff == 0xffff) {
        return 1; // assume single remote node
    } else {
        return nodeid;
    }
}
  
