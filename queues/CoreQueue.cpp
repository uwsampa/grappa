#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "CoreQueue.hpp"



// queue factory
template <class T>
CoreQueue<T>* CoreQueue<T>::createQueue() {

    #if QUSE_MCRB
    return new CoreQueue<T>();  
    #else
    return NULL;
    #endif
}


template class CoreQueue<uint64_t>;


