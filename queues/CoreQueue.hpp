#ifndef __COREQUEUE_HPP__
#define __COREQUEUE_HPP__

#include <stdint.h>
#include <stdlib.h>
    

template <class T>
class CoreQueue {
    public:

        CoreQueue(int dum);
        ~CoreQueue();

        // factory
        static CoreQueue<T>* createQueue();
        
        // blocking
        virtual bool tryProduce(const T& element) = 0;
        virtual bool tryConsume(T* element) = 0;
        
        virtual void flush() = 0;

        // sizeConsumer() + sizeProducer() should add to actual number
        // of elements in the queue. e.g. if one side can always see all
        // elements then have the other method return 0.
        virtual int sizeConsumer();
        virtual int sizeProducer();
};




#endif
