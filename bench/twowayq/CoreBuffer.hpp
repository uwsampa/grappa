#ifndef __COREBUFFER_HPP__
#define __COREBUFFER_HPP__

#include <assert.h>
#include <stdint.h>

// use queue indicators
#ifndef QUSE_MCRB
    #define QUSE_MCRB 0
#endif

#ifndef QUSE_LIBQ
    #define QUSE_LIBQ 1
#endif

#ifndef QUSE_SIMPLE
    #define QUSE_SIMPLE 0
#endif

class CoreBuffer {
    public:

        CoreBuffer(int dum);

        // factory
        static CoreBuffer* createQueue();

        // blocking
        virtual void produce(uint64_t element) = 0;
        virtual void consume(uint64_t *element) = 0;
        
        virtual bool canConsume() = 0;  
        
        virtual void flush() = 0;
};
CoreBuffer::CoreBuffer(int dum) { }

/*******************************************/
/* MCRingBuffer */
#if QUSE_MCRB
#include "MCRingBuffer.hpp"

typedef MCRingBuffer<uint64_t, 8, 32, 64> MCRingBufferD;  // TODO make this changeable


class CoreBufferMC: public CoreBuffer {


    private:
        MCRingBufferD* q;

    public:
        CoreBufferMC() 
            : CoreBuffer(0)
            , q(new MCRingBufferD()) {}
        


        inline void produce(uint64_t element) {
            while(!q->produce(element));
        }

        inline void consume(uint64_t *element) {
            while(!q->consume(element));
        }

        inline void flush() {
            q->flush();
        }

        inline bool canConsume() {
            return q->can_consume();
        }
};

#endif

/******************************************/

#if QUSE_SIMPLE
class CoreBufferSimple: public CoreBuffer {
    private:
        uint64_t data;
        bool full;
    public:
        CoreBufferSimple()
            : CoreBuffer(0)
            , data(0)
            , full(false) {}

        inline void produce(uint64_t element) {
            while (full);
            data = element;
            __sync_synchronize(); // make sure data is visible first      
            full = true; 
        }

        inline void consume(uint64_t* element) {
            while(!full);
            *element = data;  
            __sync_synchronize(); // make sure get the data before say empty
            full = false;
        }
        
        inline void flush() {
        }

        inline bool canConsume() {
            return full;
        }
};
#endif
                 


/*****************************************/
/* Liberty Queue */
#if QUSE_LIBQ
#include <sw_queue_astream.h>

class CoreBufferLQ: public CoreBuffer {

    private:
        SW_Queue q;

    public:
        CoreBufferLQ()
            : CoreBuffer(0)
            , q(sq_createQueue()) {}


        inline void produce(uint64_t element) {
            sq_produce(q, element);
        }

        inline void consume(uint64_t* element) {
            *element = sq_consume(q);
        }

        inline void flush() {
            sq_flushQueue(q);
        }

        inline bool canConsume() {
            return sq_canConsume(q);
        }
};
#endif
/***************************************/


// queue factory
CoreBuffer* CoreBuffer::createQueue() {
    assert(QUSE_MCRB + QUSE_LIBQ + QUSE_SIMPLE == 1) ;
  
    #if QUSE_MCRB
    return (CoreBuffer*) new CoreBufferMC();
    #endif

    #if QUSE_LIBQ
    return (CoreBuffer*) new CoreBufferLQ();
    #endif

    #if QUSE_SIMPLE
    return (CoreBuffer*) new CoreBufferSimple();
    #endif
}



#endif
