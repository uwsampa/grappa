#include <assert.h>
#include <stdio.h>

#include "sw_queue_astream.h"

/*
 * Create and initialize a queue.
 */
SW_Queue sq_createQueue(void)
{
  return sq_initQueue(sq_createQueueBlock(1));
}

/*
 * Creates, but does not initialize a block of queues.
 */
SW_Queue sq_createQueueBlock(unsigned queues) {

  /* Allocate the queue data structure */
  sw_queue_t *queue = (sw_queue_t *) mmap((void *) (1UL << 32),
                                          queues * sizeof(sw_queue_t),
                                          PROT_WRITE | PROT_READ,
                                          MAP_SHARED | MAP_ANONYMOUS,
                                          -1,
                                          (off_t) 0);
  if(queue == (sw_queue_t *) -1) {
    perror("sq_createQueue");
    exit(1);
  }

  return queue;
}

/*
 * Initialize a queue. sq_createQueue does this automatically.
 */
SW_Queue sq_initQueue(SW_Queue queue) {
  
  /* Allocate the queue */
  queue->data = (uint64_t *) mmap(0,
                                  sizeof(uint64_t) * QSIZE,
                                  PROT_WRITE | PROT_READ,
                                  MAP_SHARED | MAP_ANONYMOUS | MAP_32BIT,
                                  -1,
                                  (off_t) 0);
  
  if(queue->data == (void *) -1) {
    perror("sq_initQueue");
    exit(1);
  }

  /* Initialize the queue data structure */
  queue->p_data = (uint64_t) queue->data;
  queue->c_inx = 0;
  queue->c_margin = 0;
  queue->p_glb_inx = 0;
  queue->c_glb_inx = 0;
  queue->ptr_c_glb_inx = &(queue->c_glb_inx);
  queue->ptr_p_glb_inx = &(queue->p_glb_inx);

#ifdef INSTRUMENT
  queue->total_produces = 0;
  queue->total_consumes = 0;
#endif

  return queue;  
}

void sq_freeQueueBlock(SW_Queue queue, unsigned size) {

  for(unsigned i = 0; i < size; ++i) {

    /* This queue may never have been initialized. */
    if(queue[i].data) {
      
#ifdef INSTRUMENT
      fprintf(stderr, "Queue %p: %ld produces, %ld consumes\n",
              (void*)queue,
              queue->total_produces,
              queue->total_consumes);
#endif

      if(munmap(queue[i].data, sizeof(uint64_t) * QSIZE)) {
        perror("sq_freeQueueBlock");
        exit(-1);
      }
    }
  }

  if(munmap(queue, sizeof(sw_queue_t) * size)) {
    perror("sq_freeQueueBlock");
    exit(-1);    
  }
}

/*
 * Free a queue.
 */
void sq_freeQueue(SW_Queue queue)
{
  sq_freeQueueBlock(queue, 1);
}

