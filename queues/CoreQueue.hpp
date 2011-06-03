#ifndef __COREQUEUE_HPP__
#define __COREQUEUE_HPP__

#include <assert.h>
#include <stdint.h>

// use queue indicators
#ifndef QUSE_MCRB
    #define QUSE_MCRB 1
#endif

class CoreQueue {
    public:

        CoreQueue(int dum);
        ~CoreQueue();

        // factory
        static CoreQueue* createQueue();

        // blocking
        virtual bool tryProduce(uint64_t element) = 0;
        virtual bool tryConsume(uint64_t *element) = 0;
        
        virtual void flush() = 0;
};
CoreQueue::CoreQueue(int dum) { }
CoreQueue::~CoreQueue() { }

/*******************************************/
/* MCRingBuffer */
#if QUSE_MCRB
#include "MCRingBuffer.h"


class CoreQueueMC: public CoreQueue {


    private:
        MCRingBuffer* q;

    public:
        CoreQueueMC() 
            : CoreQueue(0)
            , q((MCRingBuffer*)malloc(sizeof(MCRingBuffer))) {
            
            MCRingBuffer_init(q); 
         }

         ~CoreQueueMC() {
             free(q);
         }
        


        inline bool tryProduce(uint64_t element) {
            return (bool) MCRingBuffer_produce(q, element);
        }

        inline bool tryConsume(uint64_t *element) {
            return (bool) MCRingBuffer_consume(q, element);
        }

        inline void flush() {
            MCRingBuffer_flush(q);
        }

};

#endif

/******************************************/

// queue factory
CoreQueue* CoreQueue::createQueue() {
    assert(QUSE_MCRB + QUSE_LIBQ + QUSE_SIMPLE == 1) ;
  
    #if QUSE_MCRB
    return (CoreQueue*) new CoreQueueMC();
    #endif

}



#endif
