#ifndef __GLOBALMEMORY_HPP__
#define __GLOBALMEMORY_HPP__

// for STUB
#include <queue>

#include "GmTypes.hpp"
#include "MemoryDescriptor.hpp"


typedef uint32_t nodeid_t;

typedef struct request_node_t {
    MemoryDescriptor* request;
    nodeid_t  node_id;
} request_node_t;

/*
typedef uint16_t mid_t;


typedef struct response_t {
    mid_t id;
    uint64_t data;
} response_t;

typedef struct request_t {
    uint64_t* addr;
    oper_enum op;
    mid_t id;
    uint64_t data;
} request_t;

typedef struct request_node_t {
    request_t request;
    nodeid_t  node_id;
} request_node_t;
*/

class GlobalMemory {
    private:
        nodeid_t nodeid;
        nodeid_t getNodeForDescriptor(MemoryDescriptor*);

    public:
        // for STUB
        std::queue<MemoryDescriptor*> locreq;
        std::queue<MemoryDescriptor*> remreq;


        MemoryDescriptor* getRemoteResponse();
        void sendResponse(nodeid_t to, MemoryDescriptor* resp);
        bool getRemoteRequest(request_node_t* rh);
        void sendRequest(MemoryDescriptor* md);


        uint64_t* getLocalAddress(MemoryDescriptor* md);
        bool isLocal(MemoryDescriptor* md);

        GlobalMemory(nodeid_t nid) 
            : nodeid(nid)
            , locreq()
            , remreq() {}
};


#endif
