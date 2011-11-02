
#include "MCRingBuffer.h"

void MCRingBuffer_init(MCRingBuffer * mcrb) {
  mcrb->localRead = 0;
  mcrb->nextWrite = 0;
  mcrb->wBatch = 0;
  mcrb->read = 0;
  mcrb->write = 0;
  mcrb->localWrite = 0;
  mcrb->nextRead = 0;
  mcrb->rBatch = 0;
}



/*inline*/ int MCRingBuffer_produce(MCRingBuffer * mcrb, uint64_t element) {
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


/*inline*/ void MCRingBuffer_flush(MCRingBuffer * mcrb) {
  mcrb->write = mcrb->nextWrite;
  mcrb->wBatch = 0;
}



/*inline*/ int MCRingBuffer_consume(MCRingBuffer * mcrb, uint64_t* element) {
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

/*inline*/ int MCRingBuffer_next(int x) {
  return (x + 1) & (MCRINGBUFFER_MAX - 1);
}








/* inline int MCRingBuffer_next(int x) { */
/*   return (x + 1) & (MCRINGBUFFER_MAX - 1); */
/* } */

/* inline int MCRingBuffer_produce(MCRingBuffer * mcrb, uint64_t element) { */
/*   int afterNextWrite = MCRingBuffer_next(mcrb->nextWrite); */
/*   if (afterNextWrite == mcrb->localRead) { */
/*     if (afterNextWrite == mcrb->read) { */
/*       return 0; */
/*     } */
/*     mcrb->localRead = mcrb->read; */
/*   } */
/*   mcrb->buffer[mcrb->nextWrite] = element; */
/*   mcrb->nextWrite = afterNextWrite; */
/*   mcrb->wBatch++; */
/*   if (mcrb->wBatch >= MCRINGBUFFER_BATCH_SIZE) { */
/*     mcrb->write = mcrb->nextWrite; */
/*     mcrb->wBatch = 0; */
/*   } */
/*   return 1; */
/* } */

/* inline void MCRingBuffer_flush(MCRingBuffer * mcrb) { */
/*   mcrb->write = mcrb->nextWrite; */
/*   mcrb->wBatch = 0; */
/* } */

/* inline int MCRingBuffer_consume(MCRingBuffer * mcrb, uint64_t* element) { */
/*   if (mcrb->nextRead == mcrb->localWrite) { */
/*     if (mcrb->nextRead == mcrb->write) { */
/*       return 0; */
/*     } */
/*     mcrb->localWrite = mcrb->write; */
/*   } */
/*   *element = mcrb->buffer[mcrb->nextRead]; */
/*   mcrb->nextRead = MCRingBuffer_next(mcrb->nextRead); */
/*   mcrb->rBatch++; */
/*   if (mcrb->rBatch >= MCRINGBUFFER_BATCH_SIZE) { */
/*     mcrb->read = mcrb->nextRead; */
/*     mcrb->rBatch = 0; */
/*   } */
/*   return 1; */
/* } */
