#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "GlobalMemory.hpp"
#include "MemoryDescriptor.hpp"

/* GlobalMemory impl for two delegates in same coherence domain to communicate*/

MemoryDescriptor* GlobalMemory::getRemoteResponse() {
	uint64_t data;
	bool valid = locresp->tryConsume(&data);
	if (valid) {
       return (MemoryDescriptor*) data;
   } else {
       return NULL;
   }
}
       
bool GlobalMemory::sendResponse(nodeid_t to, MemoryDescriptor* resp) {
	assert(to!=nodeid);
	if (remresp->tryProduce((uint64_t)resp)) {
    	remresp->flush(); // inefficient
        return true;
    } else {
        return false;
    }
}

bool GlobalMemory::getRemoteRequest(request_node_t* rh) {
    uint64_t data;
    bool valid = remreq->tryConsume(&data);
	if (valid) {
        rh->request = (MemoryDescriptor*) data;
        rh->node_id = 1-nodeid; // single remote node {0,1}
        return true;
    } else {
        return false;
    }
}

bool GlobalMemory::sendRequest(MemoryDescriptor* md) {
	if (locreq->tryProduce((uint64_t)md)) {
        locreq->flush();// inefficient
        return true;
    } else {
        return false;
    }
}

uint64_t* GlobalMemory::getLocalAddress(MemoryDescriptor* md) {
    return md->getAddress();
}

bool GlobalMemory::isLocal(MemoryDescriptor* md) {
    return (range_low <= md->getAddress()) && (md->getAddress() < range_high);
}
 
 
nodeid_t GlobalMemory::getNodeForDescriptor(MemoryDescriptor* md) {
    
    /* lookup addr */
    if (isLocal(md)) { // TODO real lookup
        return nodeid;
    } else {
        return 1-nodeid; // assume single remote node {0,1}
    }
}
  
