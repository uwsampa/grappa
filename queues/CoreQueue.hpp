#ifndef __COREQUEUE_HPP__
#define __COREQUEUE_HPP__

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

// use queue indicators
#ifndef QUSE_MCRB
    #define QUSE_MCRB 1
#endif

template <class T>
class CoreQueue {
    public:

        CoreQueue(int dum);
        ~CoreQueue();

        // factory
        static CoreQueue* createQueue();

        // blocking
        virtual bool tryProduce(const T& element) = 0;
        virtual bool tryConsume(T* element) = 0;
        
        virtual void flush() = 0;
};

template <class T>
CoreQueue<T>::CoreQueue(int dum) { }

template <class T>
CoreQueue<T>::~CoreQueue() { }

/*******************************************/
/* MCRingBuffer */
#if QUSE_MCRB
#include "MCRingBuffer.h"

//TODO: template the MCRingBuffer if possible
template <class T>
class CoreQueueMC: public CoreQueue<T> {


    private:
        MCRingBuffer* q;

    public:
        CoreQueueMC() 
            : CoreQueue<uint64_t>(0)
            , q((MCRingBuffer*)malloc(sizeof(MCRingBuffer))) {
            
            MCRingBuffer_init(q); 
         }

         ~CoreQueueMC() {
             free(q);
         }
        


        inline bool tryProduce(const T& element) {
            return (bool) MCRingBuffer_produce(q, (uint64_t)element); //???
        }

        inline bool tryConsume(T* element) {
            return (bool) MCRingBuffer_consume(q, (uint64_t*)element);
        }

        inline void flush() {
            MCRingBuffer_flush(q);
        }

};

#endif

/******************************************/

// queue factory
template <class T>
CoreQueue<T>* CoreQueue<T>::createQueue() {
    //assert(QUSE_MCRB + QUSE_LIBQ + QUSE_SIMPLE == 1) ;
    assert (QUSE_MCRB == 1);

    #if QUSE_MCRB
    return (CoreQueue<uint64_t>*) new CoreQueueMC<uint64_t>();  // TODO cleanup template
    #endif

}



#endif
