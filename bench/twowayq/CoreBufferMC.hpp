#ifndef __COREBUFFERMC_HPP__
#define __COREBUFFERMC_HPP__

#include "MCRingBuffer.hpp"
#include "CoreBuffer.hpp"

typedef MCRingBuffer<uint64_t, 8, 32, 64> MCRingBufferD;  // TODO make this changeable


class CoreBufferMC: public CoreBuffer {


    private:
        MCRingBufferD* q;

    public:
        CoreBufferMC() {
            q = new MCRingBufferD();
        }

        inline void produce(uint64_t element) {
            while(!q->produce(element);
        }

        inline void consume(uint64_t *element) {
            while(!q->consume(element);
        }

        inline void flush() {
            q->flush();
        }
};

#endif

