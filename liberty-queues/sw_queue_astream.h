/** ***********************************************/
/** *** SW Queue with Supporting Variable Size ****/
/** ***********************************************/
#ifndef SW_QUEUE_H
#define SW_QUEUE_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/mman.h>
#include <xmmintrin.h>
#include <unistd.h>

#include "inline.h"

#ifndef CACHELINE_SIZE
#define CACHELINE_SIZE 128
/*#define CACHELINE_SIZE 64 */ 
/* Cache line size of glacier, as reported by lmbench. Other tests,
 * namely the smtx ones, vindicate this. */
#endif /* CACHELINE_SIZE */

#ifndef CHUNK_SIZE
#ifdef DUALCORE
#define CHUNK_SIZE (1 << 8)
#else /* DUALCORE */
#define CHUNK_SIZE (1 << 14)
#endif /* DUALCORE */
#endif /* CHUNK_SIZE */

#define HIGH_CHUNKMASK ((((uint64_t) (CHUNK_SIZE - 1)) << 32))

#ifndef STREAM
#ifdef DUALCORE
#define STREAM (false)
#else /* DUALCORE */
#define STREAM (true)
#endif /* DUALCORE */
#endif /* STREAM */

#if !STREAM
#define sq_write sq_stdWrite
#else /* STREAM */
#define sq_write sq_streamWrite
#endif /* STREAM */

#ifndef QMARGIN
#define QMARGIN 4
#endif /* QMARGIN */

#ifndef QSIZE
#define QSIZE (1 << 21)
#endif /* QSIZE */

#ifndef QPREFETCH
#define QPREFETCH (1 << 7)
#endif /* QPREFETCH */

#define QMASK (QSIZE - 1)
#define HIGH_QMASK (((uint64_t) QMASK) << 32 | (uint32_t) ~0)

#define PAD(suffix, size) char padding ## suffix [CACHELINE_SIZE - (size)]

typedef struct {

  uint64_t p_data;
  PAD(1, sizeof(uint64_t));

  volatile uint32_t *ptr_c_glb_inx;
  uint32_t p_glb_inx;
  PAD(2, sizeof(volatile uint32_t *) + sizeof(uint32_t));

  uint32_t c_margin;
  uint32_t c_inx;
  PAD(3, sizeof(uint32_t) * 2);

  volatile uint32_t *ptr_p_glb_inx;
  uint32_t c_glb_inx;
  PAD(4, sizeof(volatile uint32_t *) + sizeof(uint32_t));

  uint64_t *data;

  uint64_t  total_produces;
  uint64_t  total_consumes;

  PAD(5, sizeof(volatile uint64_t *) + sizeof(uint64_t) * 2);

} sw_queue_t, *SW_Queue;

typedef void *sq_cbData;
typedef void (*sq_callback)(sq_cbData);

/* ***************************************** */
/* ******* Detail Implementation *********** */
/* ***************************************** */

SW_Queue sq_createQueue(void);
SW_Queue sq_createQueueBlock(unsigned queues);
SW_Queue sq_initQueue(SW_Queue queue);
void sq_freeQueueBlock(SW_Queue queue, unsigned size);
void sq_freeQueue(SW_Queue q);

/* ******************************************************** */
/* ***********  Preallocate Method  *********************** */
/* ******************************************************** */

/*
 * This method is faster on multi-processor machines as it bypasses the L2
 * cache.
 */
Inline void sq_streamWrite(uint64_t *addr, uint64_t value) {

  /*   __m64 mmxReg = _mm_set_pi64x((int64_t) value); */
  /*   _mm_stream_pi((__m64 *) addr, mmxReg); */

  __asm (
         "movntiq %1, (%0)\n"
         :
         : "r" (addr), "r" (value)
         );
}

/*
 * This method is faster on multi-core machines as it exploits the common L2
 * cache.
 */
Inline void sq_stdWrite(uint64_t *addr, uint64_t value) {
  *addr = value;
}

/*
 * Modulo subtraction
 */
Inline uint32_t sq_modSub(uint32_t minuend,
                          uint32_t subtrahend,
                          uint32_t mask) {
  return (minuend - subtrahend) & mask;
}

Inline uint32_t sq_pInx(SW_Queue q) {
  return (uint32_t) (q->p_data >> 32);
}

/*
 * Flush all produce values.
 *
 * sfence is slow, but necessary for streaming writes. sfence MUST proceded
 * updating p_glb_inx, otherwise there will be a race condition.
 */
Inline void sq_flushQueue(SW_Queue q)
{
#if STREAM
  _mm_sfence();
#endif

  *q->ptr_p_glb_inx = sq_pInx(q);
}

/*
 * Wait on the consumer.
 *
 * Pausing while spinning improves performance on NetBurst and improves energy
 * efficiency.
 */
Inline void sq_waitConsumer(SW_Queue q, sq_callback cb, sq_cbData cbd)
{
#ifndef NO_CON
  uint32_t threshold = (QMARGIN - 1) * QSIZE / QMARGIN;
  if(sq_modSub(sq_pInx(q), *q->ptr_c_glb_inx, QMASK) > threshold) {

    /* Blocking path */
    cb(cbd);

    while(sq_modSub(sq_pInx(q), *q->ptr_c_glb_inx, QMASK) > threshold) {
      usleep(10);
    }
  } else {

    /* Fast path */
    sq_flushQueue(q);
  }
#endif /* NO_CON */
}

/*
 * Produce a value to a queue.
 */
Inline void sq_produce(SW_Queue q, uint64_t value,
                       sq_callback cb, sq_cbData cbd)
{
#ifdef INSTRUMENT
  q->total_produces++;
#endif

  uint64_t pDataRaw = q->p_data;
  uint32_t pInx = sq_pInx(q);
  uint64_t *pData = (uint64_t *) (size_t) (uint32_t) q->p_data;
  uint64_t *ptr = pInx + pData;

  sq_write(ptr, value);
  q->p_data = (pDataRaw + (1ULL << 32)) & HIGH_QMASK;

  if(!(q->p_data & HIGH_CHUNKMASK)) {
    sq_waitConsumer(q, cb, cbd);
  }
}

#define sq_produce(Q,V) sq_produce(Q, V, (sq_callback) sq_flushQueue, Q)

/*
 * Produce two values to a queue. Do not intermingle with sq_produce.
 */
Inline void sq_produce2(SW_Queue q, uint64_t a, uint64_t b,
                        sq_callback cb, sq_cbData cbd)
{
#ifdef INSTRUMENT
  q->total_produces += 2;
#endif

  /* Otherwise, gcc couldn't constant propagate. */
  uint64_t pDataRaw = q->p_data;
  uint32_t pInx = sq_pInx(q);
  uint64_t *pData = (uint64_t *) (size_t) (uint32_t) q->p_data;
  uint64_t *ptr = pInx + pData;

  sq_write(ptr + 0, a);
  sq_write(ptr + 1, b);

  q->p_data = (pDataRaw + (2ULL << 32)) & HIGH_QMASK;

  if(!(q->p_data & HIGH_CHUNKMASK)) {
    sq_waitConsumer(q, cb, cbd);
  }
}

#define sq_produce2(Q,A,B) sq_produce2(Q, A, B, (sq_callback) sq_flushQueue, Q)

/* Functions for Consumer */

/*
 * Makes reads globally visible
 */

Inline void sq_reverseFlush(const SW_Queue q) {
  *q->ptr_c_glb_inx = q->c_inx;
}

/*
 * Wait for a produce
 */
Inline uint32_t sq_waitAllocated(const SW_Queue q, sq_callback cb, sq_cbData cbd)
{
  if(*q->ptr_p_glb_inx == q->c_inx) {

    /* Blocking path */
    cb(cbd);
    while(*q->ptr_p_glb_inx == q->c_inx) usleep(10);

  } else {
    /* Fast path */
    sq_reverseFlush(q);
  }

  return *q->ptr_p_glb_inx;
}

/*
 * Consume a 64-bit value from the queue.
 */
Inline uint64_t sq_consume(SW_Queue q, sq_callback cb, sq_cbData cbd)
{
#ifdef INSTRUMENT
  q->total_consumes++;
#endif

  if(q->c_inx == q->c_margin) {
    q->c_margin = sq_waitAllocated(q, cb, cbd);
  }

  uint64_t val = q->data[q->c_inx];

  if(QPREFETCH) {
    _mm_prefetch(q->data + q->c_inx + QPREFETCH, _MM_HINT_T0);
  }

  q->c_inx++;
  q->c_inx &= QMASK;
  return val;
}

#define sq_consume(Q) sq_consume(Q, (sq_callback) sq_reverseFlush, Q)

Inline bool sq_canConsume(const SW_Queue q) {
  return q->c_margin != q->c_inx || *q->ptr_p_glb_inx != q->c_inx;
}

/*
 * Empty the queue.
 *
 * Races with producer methods.
 */
Inline void sq_emptyQueue(SW_Queue q) {
  q->c_margin = q->c_inx = *q->ptr_p_glb_inx;
  sq_reverseFlush(q);
}

/*
 * Modulo arithmetic (llvmism)
 */
Inline bool sq_selectProducer(unsigned prevChoice, 
                              SW_Queue *queues, 
                              unsigned numQueues) {
  (void) queues;

  if( ++prevChoice >= numQueues )
    prevChoice = 0;

  return prevChoice;
}

/*
 * Modulo arithmetic (llvmism)
 */
Inline bool sq_selectConsumer(unsigned prevChoice, 
                              SW_Queue *queues, 
                              unsigned numQueues) {

  (void) queues;

  if( ++prevChoice >= numQueues )
    prevChoice = 0;

  return prevChoice;
}

#endif /* SW_QUEUE_H */
