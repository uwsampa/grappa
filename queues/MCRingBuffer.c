#include <stdio.h>
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


int MCRingBuffer_eleSize(MCRingBuffer* mcrb) {
    int write = mcrb->write;
    int writeSide = mcrb->nextWrite - write;
    writeSide = (mcrb->nextWrite < write) ? writeSide+8 : writeSide;

    return writeSide;
}

// must consume first before there is a size
int MCRingBuffer_readableSize(MCRingBuffer* mcrb) {
    int write = mcrb->write;  // mcrb->localWrite avoids reading 'write' but will not give full view of size until consume past this rBatch window
    return (write < mcrb->nextRead) ? 
                write - mcrb->nextRead + 8 : 
                write - mcrb->nextRead;
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
