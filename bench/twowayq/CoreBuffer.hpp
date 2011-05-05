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


class CoreBuffer {
    public:

        CoreBuffer(int dum);

        // factory
        static CoreBuffer* createQueue();

        // blocking
        virtual void produce(uint64_t element) = 0;
        virtual void consume(uint64_t *element) = 0;
        
        
        
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
};

#endif

/******************************************/




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
};
#endif
/***************************************/


 CoreBuffer* CoreBuffer::createQueue() {
            assert(QUSE_MCRB ^ QUSE_LIBQ);
 
            #if QUSE_MCRB
            return (CoreBuffer*) new CoreBufferMC();
            #endif

            #if QUSE_LIBQ
            return (CoreBuffer*) new CoreBufferLQ();
            #endif
        }



#endif
