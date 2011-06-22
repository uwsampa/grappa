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
};




#endif
