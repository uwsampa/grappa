#include <stdio.h>
#include <stdint.h>

#include "CoreBuffer.hpp"


int main() {
    CoreBuffer* cb = CoreBuffer::createQueue();
    cb->produce(0xDEADBEEF);
    cb->flush();
    uint64_t yum;
    cb->consume(&yum);

    printf("%lx\n", yum);
}
