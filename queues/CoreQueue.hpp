#ifndef __COREQUEUE_HPP__
#define __COREQUEUE_HPP__

#include <stdint.h>
#include <stdlib.h>
    
#define QUSE_MCRB 1

#if QUSE_MCRB
#include "MCRingBuffer.h"
#endif

template <class T>
class CoreQueue {
    private:
        #if QUSE_MCRB
        MCRingBuffer q;
        #endif

    public:

        CoreQueue();
        
        ~CoreQueue();

        // factory
        static CoreQueue<T>* createQueue();
        
        bool tryProduce(const T& element);

        bool tryConsume(T* element);

        void flush();

        int sizeConsumer();

        int sizeProducer();
};

#if QUSE_MCRB

template <class T>
CoreQueue<T>::CoreQueue() 
    : q() { // TODO MCRB is currently not templated 
    
    MCRingBuffer_init(&q); 
}
    
template <class T>
CoreQueue<T>::~CoreQueue() { }

template <class T>
inline bool CoreQueue<T>::tryProduce(const T& element) {
    return (bool) MCRingBuffer_produce(&q, element); 
}

template <class T>
inline bool CoreQueue<T>::tryConsume(T* element) {
    return (bool) MCRingBuffer_consume(&q, element);
}

template <class T>
inline void CoreQueue<T>::flush() {
    MCRingBuffer_flush(&q);
}

template <class T>
int CoreQueue<T>::sizeConsumer() {
    return MCRingBuffer_readableSize(&q);
}

template <class T>
int CoreQueue<T>::sizeProducer() {
    return MCRingBuffer_unflushedSize(&q);
}

#endif // QUSE_MCRB


#endif // header
