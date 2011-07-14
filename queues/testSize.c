
#include <assert.h>
#include <stdio.h>

#define MCRINGBUFFER_MAX 8
#define MCRINGBUFFER_BATCH_SIZE 4
#include "MCRingBuffer.h"



void printSize(MCRingBuffer* m, int rs, int es) {
    int a_rs = MCRingBuffer_readableSize(m);
    int a_es = MCRingBuffer_unflushedSize(m);
    printf("readable: %d | unflushed: %d\n", a_rs, a_es); 
    assert(a_rs==rs);
    assert(a_es==es);
}

int main() {

    MCRingBuffer m;
    MCRingBuffer_init(&m);


    printSize(&m, 0, 0);

    uint64_t data;
    assert(MCRingBuffer_consume(&m, &data)==0);

    printSize(&m, 0, 0);

    assert(MCRingBuffer_produce(&m, 10)!=0);
    printSize(&m, 0, 1);
    assert(MCRingBuffer_produce(&m, 11)!=0);
    printSize(&m, 0, 2);
    assert(MCRingBuffer_produce(&m, 12)!=0);
    printSize(&m, 0, 3);
    assert(MCRingBuffer_produce(&m, 13)!=0); // should flush
    printSize(&m, 4, 0);

    assert(MCRingBuffer_produce(&m, 24)!=0);
    printSize(&m, 4, 1); 
    assert(MCRingBuffer_consume(&m, &data)!=0);
    assert(data==10);
    printSize(&m, 3, 1);

    
    assert(MCRingBuffer_produce(&m, 35)!=0);
    printSize(&m, 3, 2);
    MCRingBuffer_flush(&m);
    printSize(&m, 5, 0);
    assert(MCRingBuffer_consume(&m, &data)!=0);
    assert(data==11);
    assert(MCRingBuffer_consume(&m, &data)!=0);
    assert(data==12);
    printSize(&m, 3, 0); 
    assert(MCRingBuffer_consume(&m, &data)!=0);
    assert(data==13);
    printSize(&m, 2, 0);
    
}
