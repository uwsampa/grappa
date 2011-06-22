#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "GlobalMemory.hpp"
#include "MemoryDescriptor.hpp"

/* GlobalMemory impl for two delegates in same coherence domain to communicate*/


MemoryDescriptor* GlobalMemory::getRemoteResponse() {
   if (locresp->size() > 0) {
       MemoryDescriptor* md = locresp->front();
       locresp->pop();
       return md;
   } else {
       return NULL;
   }
}
       
void GlobalMemory::sendResponse(nodeid_t to, MemoryDescriptor* resp) {
	assert(to!=nodeid);
	remresp->push(resp);
}

bool GlobalMemory::getRemoteRequest(request_node_t* rh) {
    if (remresp->size() > 0) {
        rh->request = remresp->front();
        rh->node_id = 1-nodeid; // single remote node {0,1}
        remresp->pop();
        return true;
    } else {
        return false;
    }
}

void GlobalMemory::sendRequest(MemoryDescriptor* md) {
    locreq->push(md);
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
    if (rand()%2==0) { // TODO real lookup
        return 1-nodeid; // assume single remote node {0,1}
    } else {
        return nodeid;
    }
}
  
