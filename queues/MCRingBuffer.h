
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

inline int MCRingBuffer_next(int x) {
  return (x + 1) & (MCRINGBUFFER_MAX - 1);
}

inline int MCRingBuffer_produce(MCRingBuffer * mcrb, uint64_t element) {
  int afterNextWrite = MCRingBuffer_next(mcrb->nextWrite);
  if (afterNextWrite == mcrb->localRead) {
    if (afterNextWrite == mcrb->read) {
      return 0;
    }
    mcrb->localRead = mcrb->read;
  }
  mcrb->buffer[mcrb->nextWrite] = element;
  mcrb->nextWrite = afterNextWrite;
  mcrb->wBatch++;
  if (mcrb->wBatch >= MCRINGBUFFER_BATCH_SIZE) {
    mcrb->write = mcrb->nextWrite;
    mcrb->wBatch = 0;
  }
  return 1;
}

inline void MCRingBuffer_flush(MCRingBuffer * mcrb) {
  mcrb->write = mcrb->nextWrite;
  mcrb->wBatch = 0;
}

inline int MCRingBuffer_consume(MCRingBuffer * mcrb, uint64_t* element) {
  if (mcrb->nextRead == mcrb->localWrite) {
    if (mcrb->nextRead == mcrb->write) {
      return 0;
    }
    mcrb->localWrite = mcrb->write;
  }
  *element = mcrb->buffer[mcrb->nextRead];
  mcrb->nextRead = MCRingBuffer_next(mcrb->nextRead);
  mcrb->rBatch++;
  if (mcrb->rBatch >= MCRINGBUFFER_BATCH_SIZE) {
    mcrb->read = mcrb->nextRead;
    mcrb->rBatch = 0;
  }
  return 1;
}

#endif
