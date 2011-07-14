
#ifndef __MCRINGBUFFER_H__
#define __MCRINGBUFFER_H__

#include <stdint.h>

#ifndef MCRINGBUFFER_SIZE_LOG
#define MCRINGBUFFER_SIZE_LOG 7
#endif

#ifndef MCRINGBUFFER_MAX
#define MCRINGBUFFER_MAX (1 << MCRINGBUFFER_SIZE_LOG)
#endif

#ifndef MCRINGBUFFER_BATCH_SIZE
#define MCRINGBUFFER_BATCH_SIZE 64
#endif

#ifndef MCRINGBUFFER_CACHE_LINE_SIZE
#define MCRINGBUFFER_CACHE_LINE_SIZE 64
#endif


typedef struct MCRingBuffer {

  char pad0[MCRINGBUFFER_CACHE_LINE_SIZE];

  // producer
  int localRead;
  int nextWrite;
  int wBatch;
  char pad1[MCRINGBUFFER_CACHE_LINE_SIZE - 3 * sizeof(int)];

  // control
  volatile int read;
  volatile int write;
  char pad2[MCRINGBUFFER_CACHE_LINE_SIZE - 2 * sizeof(int)];

  // consumer
  int localWrite;
  int nextRead;
  int rBatch;
  char pad3[MCRINGBUFFER_CACHE_LINE_SIZE - 3 * sizeof(int)];

  // constants
  int max; //should be const
  char pad4[MCRINGBUFFER_CACHE_LINE_SIZE - 1 * sizeof(int)];

  uint64_t buffer[1 << MCRINGBUFFER_SIZE_LOG];

  char pad5[MCRINGBUFFER_CACHE_LINE_SIZE];
} MCRingBuffer;


void MCRingBuffer_init(MCRingBuffer * mcrb);
//int MCRingBuffer_produce(MCRingBuffer * mcrb, uint64_t element);
//void MCRingBuffer_flush(MCRingBuffer * mcrb);
//int MCRingBuffer_consume(MCRingBuffer * mcrb, uint64_t* element);

int MCRingBuffer_next(int x);

int MCRingBuffer_produce(MCRingBuffer * mcrb, uint64_t element);

void MCRingBuffer_flush(MCRingBuffer * mcrb);

int MCRingBuffer_consume(MCRingBuffer * mcrb, uint64_t* element);

int MCRingBuffer_eleSize(MCRingBuffer * mcrb);

int MCRingBuffer_readableSize(MCRingBuffer * mcrb);

#endif
