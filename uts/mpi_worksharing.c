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
#include <mpi.h>
#include "uts_dm.h"
#include "dequeue.h"
#include "ctrk.h"

#define MPI_MAKEWORKGLOBAL_TAG 1
#define MPI_WORKREQUEST_TAG    2
#define MPI_RESPONDWORK_TAG    3
#define MPI_STATS_TAG          4

#define DEBUG 0

/* tree search stack */
static StealStack stealStack;  // only one per mpi process
static dequeue    localQueue;  // double ended queue for local work
static dequeue    globalQueue; // global work

/* These will be set in ss_init() */
static unsigned long msg_counter;
static int comm_size, comm_rank;
static const int default_polling_interval = 1;

#ifdef NONBLOCK
/* Nonblocking release handle */
MPI_Request  rls_handle = MPI_REQUEST_NULL;
void        *rls_buff   = NULL;
#endif

/*********************************************************
 *  functions                                            *
 *********************************************************/

void ss_abort(int error)
{
	MPI_Abort(MPI_COMM_WORLD, error);
}

int getWorkQueueId(void)
{
	return comm_size - 1;
}

char * ss_get_par_description()
{
	return "MPI Workmanager";
}

/* release k values from bottom of local stack */
void release(StealStack *s)
{
	StealStackNode *node;
	void *work;
	int work_queue_id;
#ifdef NONBLOCK
	MPI_Status status;
#endif

	/* Get a node from the back of the queue to release */
	node = deq_popBack(localQueue);

	if (node) {
		/* If this node is not full we can't release it. */
		if (node->head != s->chunk_size)
			ss_error("release(): Attempted to release a non-full node", 1);
		work = node->work;
		work_queue_id = getWorkQueueId();

		ctrk_put(comm_rank, work);
#ifdef NONBLOCK
		if (rls_handle != MPI_REQUEST_NULL) {
			MPI_Wait(&rls_handle, &status);
			free(rls_buff);
		}

		rls_handle = work;

		MPI_Isend(rls_handle, s->work_size*s->chunk_size, MPI_BYTE, work_queue_id,
			MPI_MAKEWORKGLOBAL_TAG, MPI_COMM_WORLD, &rls_handle);
#else
		MPI_Send(work, s->work_size*s->chunk_size, MPI_BYTE, work_queue_id,
			MPI_MAKEWORKGLOBAL_TAG, MPI_COMM_WORLD);
#endif  /* NONBLOCK */

		++msg_counter;
		free(node);
		s->localWork -= s->chunk_size;
		s->nRelease ++;
	}
	else
		ss_error("release(): Do not have a chunk to release", 1);
}


/**
 * ensure local work exists, find it if it doesnt
 *  returns process id where work is stolen from if no can be found locally
 *  returns -1 if no local work exists and none could be stolen
 **/
int ensureLocalWork(StealStack *s)
{
	int work_queue_id = getWorkQueueId();
	int work_rcv;
	void *work_ptr;
	StealStackNode *node;
	MPI_Status status;

	if (s->localWork < 0)
		ss_error("ensureLocalWork(): localWork count is less than 0!", 2);

	/* If no more work */
	if (s->localWork == 0) {
		ss_setState(s, SS_SEARCH);

		work_ptr = malloc(s->work_size*s->chunk_size);
		node = (StealStackNode*)malloc(sizeof(StealStackNode));

		if (!work_ptr || !node)
			ss_error("ensureLocalWork(): Out of virtual memory.", 10);

		/* Send a work request - will block until work is available or program terminates. */
#ifdef NONBLOCK
		MPI_Wait(&rls_handle, &status);
#endif
		++msg_counter;  // Increase our timestamp
		MPI_Send(&msg_counter, 1, MPI_LONG, work_queue_id, MPI_WORKREQUEST_TAG, MPI_COMM_WORLD);
		MPI_Recv(work_ptr, s->work_size*s->chunk_size, MPI_BYTE, work_queue_id, MPI_RESPONDWORK_TAG,
			MPI_COMM_WORLD, &status);

		// FIXME: Safe to assume chunk is full? 
		node->head = s->chunk_size;
		node->work = work_ptr;

		// FIXME: Should we check the tag instead?
		MPI_Get_count(&status, MPI_BYTE, &work_rcv);

		if (work_rcv == 0) {
			/* No more work, time to terminate */
			if (DEBUG & 2) printf("Thread %d terminating\n", comm_rank); fflush(NULL);
			return -1;
		}
		else if (work_rcv != s->work_size * s->chunk_size) {
			ss_error("ensureLocalWork(): Work received size does not equal chunk size", 10);
		}

		ctrk_get(comm_rank, work_ptr);

		s->nSteal++;    
		s->localWork += s->chunk_size;
#ifdef TRACE
		ss_markSteal(s, getWorkQueueId());
#endif

		/* Push stolen work onto the back of the queue */
		deq_pushBack(localQueue, node);

		return work_queue_id;
	}

	return 0;  //local work already exists
}


/* restore stack to empty state */
void mkEmpty(StealStack *s)
{
	deq_mkEmpty(localQueue);
	deq_mkEmpty(globalQueue);

	s->localWork = 0;
	s->globalWork = 0;
}

struct waiting_entry { int flag; void *buf; };

void doWorkQueueManager(int size, StealStack *s)
{
	MPI_Request request[size*3]; //make one array so we can do a Waitall on all comm
	MPI_Request *req_make_global = &request[0];
	MPI_Request *req_work_request = &request[size];
	MPI_Request *req_response = &request[2*size];
	MPI_Status request_status; //, send_status;
	MPI_Status wait_all_status[3*size];
	void *shared_work_buf[size];
	unsigned long work_request_buf[size];
	int flag, who, i;
	struct waiting_entry waiting[size];
	unsigned long timestamps[size];
	unsigned long msg_counts[size];
	int work_response_send_count=0;
	int done=0;

#ifdef TRACE_RELEASES
	/* Track releases */
	ss_setState(s, SS_WORK);
#else
	/* Attribute the WQM's time to overhead */
	ss_setState(s, SS_WORK);
	ss_setState(s, SS_IDLE);
#endif 

	/* Init the receieve buffers */
	for(i = 0; i < size; i++) {
		waiting[i].flag = 0;   /*init waiting to not waiting*/
		waiting[i].buf  = NULL;    /*init waiting to not waiting*/
		timestamps[i] = 0;
		msg_counts[i] = 0;
		shared_work_buf[i] = malloc(s->work_size*s->chunk_size);
	}

	/* Setup non-block recieves for communicating with workers */
	for(i=0; i < size; i++) {
		/* Listen for work releases */
		MPI_Irecv(shared_work_buf[i], s->work_size*s->chunk_size, MPI_BYTE, i,
				MPI_MAKEWORKGLOBAL_TAG, MPI_COMM_WORLD, &req_make_global[i]);

		/* Listen for work requests (A WORKREQUEST should be the chunksize requested) */
		MPI_Irecv(&work_request_buf[i], 1, MPI_LONG, i, MPI_WORKREQUEST_TAG,
				MPI_COMM_WORLD, &req_work_request[i]);
	}

	/** BEGIN WORK MANAGEMENT LOOP */
	while(!done) {
		/* Wait for someone to send work or to request work */
		MPI_Waitany(2*size, request, &who, &request_status);

		/* Sending shared work to the queue */
		if(who < size) {
			void *w = malloc(s->work_size*s->chunk_size);

#ifdef TRACE_RELEASES
			/* Mark this release as a "steal" event */
			ss_markSteal(s, who);
			ss_setState(s, SS_SEARCH);
			ss_setState(s, SS_WORK);
#endif 
			/* Update timestamp */
			msg_counts[who]++;

			memcpy(w, shared_work_buf[who], s->work_size*s->chunk_size);
			deq_pushFront(globalQueue, w);
			s->globalWork += s->chunk_size;

			MPI_Irecv(shared_work_buf[who], s->work_size*s->chunk_size, MPI_BYTE, who,
					MPI_MAKEWORKGLOBAL_TAG, MPI_COMM_WORLD, &req_make_global[who]);
		}

		/* Requesting shared work from the queue */
		else { // (who >= size)
			who -= size;
			/* mark this id is waiting for work */
			waiting[who].flag = 1;

			/* Update timestamp */
			msg_counts[who]++;
			timestamps[who] = work_request_buf[who];
			/* This should be an invariant.. */
			if (timestamps[who] < msg_counts[who]) {
				ss_error("WQM: message delivery failure!\n", 10);
			}


			MPI_Irecv(&work_request_buf[who], 1, MPI_LONG, who, MPI_WORKREQUEST_TAG, MPI_COMM_WORLD, &req_work_request[who]);
		}

		/* finish last round of sends before start to send more data */
		if (work_response_send_count) {
			MPI_Waitall(work_response_send_count, req_response, wait_all_status);

                        // Free all the buffers used in the last round
                        for (i = 0; i < size; i++) {
                          if (waiting[i].buf != NULL) {
                            free(waiting[i].buf);
                            waiting[i].buf = NULL;
                          }
                        }
		}

		/* Attempt to send work to everyone who is waiting. */
		work_response_send_count = 0;
		for (i = 0; i < size; i++) {
			if (waiting[i].flag && !deq_isEmpty(globalQueue)) {
				void* work_ptr = deq_popFront(globalQueue);

				MPI_Isend(work_ptr, s->work_size*s->chunk_size, MPI_BYTE, i,
						MPI_RESPONDWORK_TAG, MPI_COMM_WORLD, &req_response[work_response_send_count]);

				work_response_send_count++;
				s->globalWork -= s->chunk_size;
				waiting[i].flag = 0;
                                waiting[i].buf  = work_ptr;
			}
		}

		/** Check for termination **/
		/* If everyone is still waiting and there are no outstanding messages
		   then we are done.  */
		done = 1;
		for(i=0; i < size; i++) {
			if(!waiting[i].flag || (msg_counts[i] != timestamps[i])) {
				done=0;
				break; //no need to check everyone else
			}
		}

		/* Sanity check */
		if(done && !deq_isEmpty(globalQueue)) {
			ss_error("WQM: Something evil happened.  We are terminating but I still have work!", 13);
		}
	} /* END: while (!done) */

	if (DEBUG & 2) printf("Queue Manager: We are done.  Letting everyone know.\n");

	/* This is a sanity test to make sure our prioritazation above works.  If this testany were to
	   return true, the cancels below would error out. */
	MPI_Testany(2*size, request, &who, &flag, &request_status);
	if (flag) {
		ss_error("WQM: Attempted to terminate with inbound work!", 13);
	}

	/* Cancel the outstanding MPI_Irecvs */
	for (i = 0; i < size; i++) {
		MPI_Cancel(&req_make_global[i]);
		MPI_Cancel(&req_work_request[i]);
	}

	/* send a msg to everyone that no work exists, everyone should be waiting on an MPI_recv here */
	work_response_send_count = 0;
	for(i=0; i < size; i++) {
		MPI_Isend(NULL, 0, MPI_BYTE, i, MPI_RESPONDWORK_TAG, MPI_COMM_WORLD, &req_response[i]);
		work_response_send_count++;
	}

	MPI_Waitall(work_response_send_count, req_response, wait_all_status);

	ss_setState(s, SS_IDLE);
}


/* initialize the stack */
StealStack* ss_init(int *argc, char ***argv)
{
	StealStack* s = &stealStack; //only one s per thread needed

	/* Start up MPI */
	MPI_Init(argc, argv);
	
	MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);

        if (comm_rank == 0 && comm_size == 1)
          ss_error("Error: Worksharing requires 2 or more MPI processes (1 work server, >= 1 worker)", 1);

	/* Reset timestamps */
	msg_counter = 0;

        s->globalWork = 0;
        s->localWork  = 0;

	s->nNodes = 0;
        s->nLeaves = 0;
	s->nAcquire = 0;
	s->nRelease = 0;
	s->nSteal = 0;
	s->nFail = 0;

        s->maxStackDepth = 0;
        s->maxTreeDepth  = 0;

	localQueue = deq_create();
	globalQueue = deq_create();
	mkEmpty(s);

	return s;
}

void ss_finalize()
{
	MPI_Finalize();
}

int ss_start(int work_size, int chunk_size)
{
	StealStack* s = &stealStack;
	double t1, t2;
	
	s->work_size = work_size;
	s->chunk_size = chunk_size;

	if (polling_interval == 0) {
		// Set a default polling interval
		polling_interval = default_polling_interval;
	}

	if (comm_rank == 0)
		printf("Work-Sharing release interval = %d\n", polling_interval);

	if(comm_rank == getWorkQueueId()){
		//this thread does not do real work
		t1 = wctime();
		doWorkQueueManager(comm_size - 1, s);
		t2 = wctime();
		s->walltime = t2 - t1;
#ifdef TRACE
		s->startTime = t1;
		s->sessionRecords[SS_IDLE][s->entries[SS_IDLE] - 1].endTime = t2;
#endif
		return 0;
	}

	return 1;
}

void ss_stop()
{
	return;
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
	memcpy(((uint8_t*)n->work)+(s->work_size*n->head), node_c, s->work_size);

	n->head++;
	s->localWork++;
	s->maxStackDepth = max(s->globalWork + s->localWork, s->maxStackDepth);
	
	/* If there is sufficient local work, release a chunk to the global queue */
	if (s->localWork > 2*s->chunk_size) {
		if (s->nNodes % polling_interval == 0) {
#ifndef TRACE
			ss_setState(s, SS_OVH);
#endif
			release(s);
#ifndef TRACE
			ss_setState(s, SS_WORK);
#endif
		}
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

	/* Call ensureLocalWork() to make sure there is work on our local queue.
	 * If the local queue is empty, this will get work from the global queue */
	if (ensureLocalWork(s) == -1) {
		if (DEBUG & 1) printf("StealStack::pop - stack is empty and no work can be found");
		ss_setState(s, SS_IDLE);
		node_c = NULL;
		return STATUS_TERM;
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

	return STATUS_HAVEWORK;
}

/* Returns true to the thread that has the stats 
 * s should be able to hold NUM_THREADS stealstacks! */
int ss_gather_stats(StealStack *s, int *count)
{
	int i;
	MPI_Status status;

	*count = comm_size;

	/* Gather stats onto thread 0 */
	if (comm_rank > 0) {
		MPI_Send(&stealStack, sizeof(StealStack), MPI_BYTE, 0, MPI_STATS_TAG, MPI_COMM_WORLD);
		return 0;
	} else {
		memcpy(s, &stealStack, sizeof(StealStack));
		for(i = 1; i < *count; i++) {
			MPI_Recv(&(s[i]), sizeof(StealStack), MPI_BYTE, i, MPI_STATS_TAG, MPI_COMM_WORLD, &status);
		}
	}

	return 1;
}

int ss_get_thread_num()
{
	return comm_rank;
}

int ss_get_num_threads()
{
	return comm_size;
}
