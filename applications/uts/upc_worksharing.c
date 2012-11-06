/*
 *         ---- The Unbalanced Tree Search (UTS) Benchmark ----
 *  
 *  Copyright (c) 2010 See AUTHORS file for copyright holders
 *
 *  This file is part of the unbalanced tree search benchmark.  This
 *  project is licensed under the MIT Open Source license.  See the LICENSE
 *  file for copyright and licensing information.
 *
 *  UTS is a collaborative project between researchers at the University of
 *  Maryland, the University of North Carolina at Chapel Hill, and the Ohio
 *  State University.  See AUTHORS file for more information.
 *
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "dequeue.h"
#include "shared_dequeue.h"
#include "uts_dm.h"
#include <upc_relaxed.h>

#define DEBUG 0

#ifdef __BERKELEY_UPC__
/* Interval in nNodes that bupc_pull() is called in ss_get_work() */
static const int bupc_polling_factor = 2;
static int bupc_polling_interval; // = chunk_size/bupc_polling_factor
                                  // Interval for working nodes to call bupc_poll
#endif

/* tree search stack */
shared StealStack stealStack[THREADS];
static shr_dequeue shared globalQueue;
static dequeue localQueue;
static upc_lock_t * gq_lock;

/* termination detection flags */
static strict shared int cb_count;
static strict shared int cb_done[THREADS];
static upc_lock_t * cb_lock;

#ifdef __BERKELEY_UPC__
/* Backoff sleep time for nodes searching for work in usec */
const unsigned long baseline_search_timeout = 100;
unsigned long search_timeout; // = baseline_search_timeout * THREADS
#else
/* Use sleeptime on systems that aren't running BUPC */
const unsigned long upc_sleeptime = 100000;
#endif


/***************************************************************************/
/* Global Queue Load Balancing                                             */
/***************************************************************************/
void put_global_queue(void *work, int size)
{
	shared void *shr_data;

	shr_data = upc_alloc(size);
	upc_memput(shr_data, work, size);
	shr_deq_pushFront(globalQueue, shr_data);
}

void *get_global_queue(int size)
{
	shared void *shr_data;
	void *local_data;
	
	if (shr_deq_isEmpty(globalQueue))
		return NULL;
	else {
		local_data = malloc(size);
		if (!local_data)
			ss_error("Out of virtual memory!", 1);

		shr_data = shr_deq_popFront(globalQueue);
		upc_memget(local_data, shr_data, size);
#ifdef TRACE
		ss_markSteal((StealStack*)&stealStack[MYTHREAD], upc_threadof(shr_data));
#endif
		upc_free(shr_data);
		return local_data;
	}
}

/* release k values from bottom of local stack */
void release(StealStack *s)
{
	StealStackNode *node;
	void *work;

	/* Get a node from the back of the queue to release */
	node = deq_popBack(localQueue);

	if (node) {
		/* If this node is not full we can't release it. */
		if (node->head != s->chunk_size)
			ss_error("release(): Attempted to release a non-full node", 1);
		work = node->work;

		/* Push work onto the global queue */
		upc_lock(gq_lock);
		put_global_queue(work, s->chunk_size*s->work_size);
		upc_unlock(gq_lock);
	
		free(node);
		s->localWork -= s->chunk_size;
		s->nRelease++;
	}
	else {
		ss_error("release(): Do not have a chunk to release", 1);
	}
}


/**
 * Ensure local work exists, find it if it doesnt.
 *   Return STATUS_TERM - If termination is detected when searching for work
 *   Return STATUS_HAVEWORK - If local work exists
 **/
int ensureLocalWork(StealStack *s)
{
	int work_rcv, i;
	long k = s->chunk_size;
	void *work_ptr = NULL;
	StealStackNode *node;

	if (DEBUG & 1) printf("Thread %d: Entered ensureLocalWork(), localWork = %d, deq_length(localQueue) = %d\n", MYTHREAD,
		s->localWork, deq_length(localQueue));

	if (s->localWork < 0)
		ss_error("ensureLocalWork(): localWork count is less than 0!", 2);

	/* If no more work */
	if (s->localWork == 0) {
		ss_setState(s, SS_SEARCH);

		node = malloc(sizeof(StealStackNode));

		//if (!work_ptr || !node)
		if (!node)
			ss_error("ensureLocalWork(): Out of virtual memory.", 10);


		/* Check the global queue for work.  Block here if no work is available. */
		if (DEBUG & 1) printf("Thread %d: Trying cb_lock\n", MYTHREAD);
		
		upc_lock(cb_lock);
		upc_lock(gq_lock);
		/* Increment the count of idle PEs and check for termination */
		if (++cb_count == THREADS && shr_deq_isEmpty(globalQueue)) {
			/* If we have termination, set the local done flag on all nodes */
			for (i = 0; i < THREADS; i++) {
				cb_done[i] = 1;
			}
		}
		upc_unlock(gq_lock);
		upc_unlock(cb_lock);

		if (DEBUG & 1) printf("Thread %d: Entering barrier\n", MYTHREAD);

		while (!cb_done[MYTHREAD]) {
			if (!shr_deq_isEmpty(globalQueue)) {
				// upc_lock_attempt(gq_lock) may be more scalable here since it prevents queueing on the lock
				if (upc_lock_attempt(gq_lock)) {
//				upc_lock(gq_lock);

				if (DEBUG & 1) printf("Thread %d: Attempting to take work from the globalQueue\n", MYTHREAD);
				work_ptr = get_global_queue(s->chunk_size*s->work_size);

				if (work_ptr != NULL) {
					cb_count--;
					upc_unlock(gq_lock);
					break;
				}
					
				upc_unlock(gq_lock);
				}
			}

#ifdef __BERKELEY_UPC__
			for (i = 0; i < search_timeout; i++) {
				bupc_poll();
			}
#else
			usleep(upc_sleeptime);
#endif
		}
		
		if (DEBUG & 1) printf("Thread %d: Leaving barrier\n", MYTHREAD);

		// Check for termination		
		if (cb_done[MYTHREAD])
			return STATUS_TERM;

		// All chunks from the GQ are full and contain only unexpored nodes. 
		node->head = s->chunk_size;
		node->work = work_ptr;

		s->nSteal++;    
		s->localWork += s->chunk_size;

		/* Push stolen work onto the back of the queue */
		deq_pushBack(localQueue, node);
	}

	/* Local work exists */
	return STATUS_HAVEWORK;
}



/***************************************************************************/
/* API FUNCTIONS                                                           */
/***************************************************************************/

char * ss_get_par_description()
{
#ifdef __BERKELEY_UPC__
	return "Berkeley UPC";
#else
	return "UPC";
#endif
}


/* initialize the stack */
StealStack* ss_init(int *argc, char ***argv)
{
	StealStack* s = (StealStack*) &stealStack[MYTHREAD];

	//s->stackLock = NULL; //no stack-locks with global queue
	s->nNodes = 0;
        s->nLeaves = 0;
	s->nAcquire = 0;
	s->nRelease = 0;
	s->nSteal = 0;
	s->nFail = 0;
	localQueue = deq_create();
	deq_mkEmpty(localQueue);
	s->localWork = 0;

	return s;
}

int ss_start(int work_size, int chunk_size)
{
	StealStack* s = (StealStack*) &stealStack[MYTHREAD];

	s->work_size = work_size;
	s->chunk_size = chunk_size;

	if (polling_interval == 0) {
		// Set a default polling interval
		polling_interval = 1;
	}

	/* Init the termination detection stuff */
	cb_lock = upc_all_lock_alloc();
	cb_count = 0;
	cb_done[MYTHREAD] = 0;

#ifdef __BERKELEY_UPC__
	search_timeout = baseline_search_timeout * THREADS;
	/* Add 1 so that CS = 1 is OK */
	bupc_polling_interval = (chunk_size + 1)/bupc_polling_factor;
#endif

	/* Allocate a lock for the global queue */
	gq_lock = upc_all_lock_alloc();

	/* Initialize the global work queue */
	if(MYTHREAD == 0) {
		globalQueue = shr_deq_create();
		shr_deq_mkEmpty(globalQueue);
	}

	// Line up for timing
	upc_barrier;
	return 1;
}

void ss_stop()
{
	upc_barrier;
	return;
}

void ss_finalize()
{
	return;
}

void ss_abort(int error)
{
	upc_global_exit(error);
}

/* local push */
void ss_put_work(StealStack *s, void* node_c)
{
	StealStackNode* n;
	void *work;

	/* If the stack is empty, push an empty StealStackNode. */
	if (deq_isEmpty(localQueue)) {
		n = malloc(sizeof(StealStackNode));
		work = malloc(s->chunk_size*s->work_size);
		if (!n || !work) ss_error("ss_put_work(): Out of virtual memory", 3);
		n->work = work;
		n->head = 0;
		deq_pushFront(localQueue, n);
	}

	/* Try to add more work to chunk on our local queue */
	n = deq_peekFront(localQueue);

	/* If the current StealStackNode is full, push a new one. */
	if (n->head == s->chunk_size) {
		n = malloc(sizeof(StealStackNode));
		work = malloc(s->chunk_size*s->work_size);
		if (!n || !work) ss_error("ss_put_work(): Out of virtual memory", 3);
		n->head = 0;
		n->work = work;
		deq_pushFront(localQueue, n);
	}
	else if (n->head > s->chunk_size)
		ss_error("ss_put_work(): Block has overflowed!", 3);
 
	/* Copy the work to the local queue, increment head */
	//FIXME: What if instead we did a "get_buffer" to return a buffer that you can store
	// a unit of work in.  Then we wouldn't need to memcpy and free.

	memcpy(((uint8_t*)n->work)+(s->work_size*n->head), node_c, s->work_size);

	n->head++;
	s->localWork++;
	s->maxStackDepth = max(s->globalWork + s->localWork, s->maxStackDepth);
	
	/* If there is sufficient local work, release a chunk to the global queue */
	if (s->localWork > 2*s->chunk_size && s->nNodes % polling_interval == 0) {
		release(s);
	}
}


/**
   if no work is found no local work is found, and
   none can be stolen, return original s and c is null
   if work is found, return the StealStack and set c to 
   return node
**/
int ss_get_work(StealStack *s, void* node_c)
{
	StealStackNode* n;
	int status;

	/* Call ensureLocalWork() to make sure there is work on our local queue.
	 * If the local queue is empty, this will get work from the global queue */
	if ((status = ensureLocalWork(s)) != STATUS_HAVEWORK) {
		if (DEBUG & 1) printf("StealStack::pop - stack is empty and no work can be found\n");

		ss_setState(s, SS_IDLE);
		node_c = NULL;
		return status;
	}

	/* We have work */
	ss_setState(s, SS_WORK);

	/* ensureLocalWork() ensures that the local work queue is not empty,
	 * so at this point we know there must be work available */
	n = deq_peekFront(localQueue);

	/* head always points at the next free entry in the work array */
	n->head--;
	memcpy(node_c,((uint8_t*)n->work) + ((s->work_size)*(n->head)),s->work_size);

	/* This chunk in the queue is empty so dequeue it */
	if(n->head == 0) {
		deq_popFront(localQueue);
		free(n->work);
		free(n);
	}
	else if (n->head < 0) {
		/* This happens if an empty chunk is left on the queue */
		fprintf(stderr, "ss_get_work(): called with n->head = 0, s->localWork=%d or %d (mod %d)\n",
			s->localWork, s->localWork % s->chunk_size, s->chunk_size);
		ss_error("ss_get_work(): Underflow!", 5);
	}

	s->nNodes++;
	s->localWork--;

#ifdef __BERKELEY_UPC__
	if (s->nNodes % bupc_polling_interval == 0)
		bupc_poll();
#endif

	return STATUS_HAVEWORK;
}

int ss_get_thread_num()
{
	return MYTHREAD;
}

int ss_get_num_threads()
{
	return THREADS;
}

int ss_gather_stats(StealStack *s, int *count)
{
	int i;
	*count = THREADS;

	if (MYTHREAD == 0) {
		for (i = 0; i < THREADS; i++) {
			upc_memget(&s[i], &stealStack[i], sizeof(StealStack));
		}
		return 1; // I have the stats
	}

	return 0; // I don't have the stats
}
