#ifndef __COREBUFFERLQ_HPP__
#define __COREBUFFERLQ_HPP__

#include "CoreBuffer.hpp"


//#include <sw_queue_greenery.h>  // <--- NOT sure if i want to do this (green threads)
#include <sw_queue_astream.h>

class CoreBufferLQ: public CoreBuffer {

    private:
        SWQueue q;

    public:
        CoreBufferLQ() {
            q = sq_createQueue();
        }

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
