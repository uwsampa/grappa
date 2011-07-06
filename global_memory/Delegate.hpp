#ifndef __DELEGATE_HPP__
#define __DELEGATE_HPP__

#include <list>

#include "MemoryDescriptor.hpp"
#include "CoreQueue.hpp"
#include "GmTypes.hpp"
#include "ga++.h"

// structure to hold request handles
// necessary when Delegate is on return path
typedef struct request_handle {
    ga_nbhdl_t handle;
    MemoryDescriptor* md;
} request_handle_t;

typedef std::list<request_handle_t> hlist_t;

class Delegate {
    private:
        CoreQueue<uint64_t>** inQs;
        CoreQueue<uint64_t>** outQs;
        uint32_t numLocal;

    public:
        GA::GlobalArray* vertices;
        hlist_t handles;
        
        int activeCount;
        Delegate(CoreQueue<uint64_t>* qs_out[], CoreQueue<uint64_t>* qs_in[], uint32_t numLocal, GA::GlobalArray* vertices);
        void run();
        
        // should only be called by same thread that is doing run()
        void decrementActive();
        bool doContinue();

};


#endif
