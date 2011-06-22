#ifndef __DELEGATE_HPP__
#define __DELEGATE_HPP__

#include "CoreQueue.hpp"
#include "GlobalMemory.hpp"
#include "GmTypes.hpp"

class Delegate {
    private:
        CoreQueue<uint64_t>** inQs;
        CoreQueue<uint64_t>** outQs;
        uint32_t numLocal;
        GlobalMemory* global_mem;

    public:
        Delegate(CoreQueue<uint64_t>* qs_out[], CoreQueue<uint64_t>* qs_in[], uint32_t numLocal, GlobalMemory* gm);
        void run();
        


};

   
    


#endif
