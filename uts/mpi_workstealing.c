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

#define DBG_GEN    1
#define DBG_CHUNK  2
#define DBG_TOKEN  4
#define DBG_MSGCNT 8

//#define DEBUG_LEVEL (DBG_GEN | DBG_CHUNK)
#define DEBUG_LEVEL 0

#define DEBUG(dbg_class, command) if (DEBUG_LEVEL & (dbg_class)) { command; }

enum uts_tags { MPIWS_WORKREQUEST = 1, MPIWS_WORKRESPONSE, MPIWS_TDTOKEN, MPIWS_STATS };
enum colors { BLACK = 0, WHITE, PINK, RED };
char *color_names[] = { "BLACK", "WHITE", "PINK", "RED" };

typedef struct {
	enum colors color;
	long send_count;
	long recv_count;
} td_token_t;

/** Global State **/
static StealStack  stealStack;    // Stores the UTS-related stats
static dequeue     localQueue;    // double ended queue of local only work
static enum colors my_color;      // Ring-based termination detection 
static int         last_steal;    // Rank of last thread stolen from
       long        chunks_recvd;  // Total messages received
       long        chunks_sent;   // Total messages sent
       long        ctrl_recvd;    // Total messages received
       long        ctrl_sent;     // Total messages sent

/** Global Parameters: Set in ss_init() **/
static int comm_size, comm_rank;

/** Adaptive Polling Interval Parameters **/
static int pollint_default    = 0; // 0 is adaptive
static int pollint_isadaptive = 0;
static int pollint_min        = 1;
static int pollint_max        = 1024;

#define POLLINT_GROW   polling_interval + 4
#define POLLINT_SHRINK pollint_min / 2
//#define POLLINT_SHRINK pollint_min

/** Global Communication handles **/
static MPI_Request  wrin_request;  // Incoming steal request
static MPI_Request  wrout_request; // Outbound steal request
static MPI_Request  iw_request;    // Outbound steal request
static MPI_Request  td_request;    // Term. Detection listener

/** Global Communication Buffers **/
static long        wrin_buff;       // Buffer for accepting incoming work requests
static long        wrout_buff;      // Buffer to send outgoing work requests
static void       *iw_buff;         // Buffer to receive incoming work
static td_token_t  td_token;        // Dijkstra's token


/*********************************************************
 *  functions                                            *
 *********************************************************/

void * release(StealStack *s);

/* Fatal error */
void ss_abort(int error)
{
	MPI_Abort(MPI_COMM_WORLD, error);
}

char * ss_get_par_description()
{
	return "MPI Workstealing";
}

/** Make progress on any outstanding WORKREQUESTs or WORKRESPONSEs */
void ws_make_progress(StealStack *s)
{
	MPI_Status  status;
	int         flag, index;
	void       *work;

	// Test for incoming work_requests
	MPI_Test(&wrin_request, &flag, &status);

	if (flag) {
		// Got a work request
		++ctrl_recvd;
		
		/* Repost that work request listener */
		MPI_Irecv(&wrin_buff, 1, MPI_LONG, MPI_ANY_SOURCE, MPIWS_WORKREQUEST, MPI_COMM_WORLD,
			       	&wrin_request);

		index = status.MPI_SOURCE;
		
		/* Check if we have any surplus work */
		if (s->localWork > 2*s->chunk_size) {
			work = release(s);
			DEBUG(DBG_CHUNK, printf(" -Thread %d: Releasing a chunk to thread %d\n", comm_rank, index));
			++chunks_sent;
			MPI_Send(work, s->chunk_size*s->work_size, MPI_BYTE, index,
				MPIWS_WORKRESPONSE, MPI_COMM_WORLD);
			free(work);

			/* If a node to our left steals from us, our color becomes black */
			if (index < comm_rank) my_color = BLACK;
		} else {
			// Send a "no work" response
			++ctrl_sent;
			MPI_Send(NULL, 0, MPI_BYTE, index, MPIWS_WORKRESPONSE, MPI_COMM_WORLD);
		}

		if (pollint_isadaptive && s->localWork != 0)
			polling_interval = max(pollint_min, POLLINT_SHRINK);
	} else {
		if (pollint_isadaptive && s->localWork != 0)
			polling_interval = min(pollint_max, POLLINT_GROW);
        }

	return;
}


/* release k values from bottom of local stack */
void * release(StealStack *s)
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

		ctrk_put(comm_rank, node->work);

	    free(node);
	    s->localWork -= s->chunk_size;
	    s->nRelease ++;

	    return work;
	}
	else {
		ss_error("release(): Do not have a chunk to release", 1);
		return NULL; // Unreachable
	}
}


/**
 * ensure local work exists, find it if it doesnt
 *  returns process id where work is stolen from if no can be found locally
 *  returns -1 if no local work exists and none could be stolen
 **/
int ensureLocalWork(StealStack *s)
{
	MPI_Status  status;
	int         flag;

	if (s->localWork < 0)
		ss_error("ensureLocalWork(): localWork count is less than 0!", 2);

	/* If no more work */
	while (s->localWork == 0) {
		if (comm_size == 1) return -1;

		if (my_color == PINK)
			ss_setState(s, SS_IDLE);
		else
			ss_setState(s, SS_SEARCH);
		
		/* Check if we should post another steal request */
		if (wrout_request == MPI_REQUEST_NULL && my_color != PINK) {
			if (iw_buff == NULL) {
				iw_buff = malloc(s->chunk_size*s->work_size);
				if (!iw_buff)
					ss_error("ensureLocalWork(): Out of memory\n", 5);
			}
		
			/* Send the request and wait for a work response */
			last_steal = (last_steal + 1) % comm_size;
			if (last_steal == comm_rank) last_steal = (last_steal + 1) % comm_size;

			DEBUG(DBG_CHUNK, printf("Thread %d: Asking thread %d for work\n", comm_rank, last_steal));

			++ctrl_sent;

			MPI_Isend(&wrout_buff, 1, MPI_LONG, last_steal, MPIWS_WORKREQUEST, MPI_COMM_WORLD, &wrout_request);
			MPI_Irecv(iw_buff, s->chunk_size*s->work_size, MPI_BYTE, last_steal, MPIWS_WORKRESPONSE,
					MPI_COMM_WORLD, &iw_request);
		}

		// Call into the stealing progress engine and update our color
		ws_make_progress(s);
	
		// Test for incoming work
		MPI_Test(&iw_request, &flag, &status);

		if (flag && wrout_request != MPI_REQUEST_NULL) {
			int work_rcv;

			MPI_Get_count(&status, MPI_BYTE, &work_rcv);
		
			if (work_rcv > 0) {
				StealStackNode *node;
		
				++chunks_recvd;
				DEBUG(DBG_CHUNK, printf(" -Thread %d: Incoming Work received, %d bytes\n", comm_rank, work_rcv));
	
				if (work_rcv != s->work_size * s->chunk_size) {
					ss_error("ws_make_progress(): Work received size does not equal chunk size", 10);
				}
	
				/* Create a new node to attach this work to */
				node = (StealStackNode*)malloc(sizeof(StealStackNode));
	
				if (!node)
					ss_error("ensureLocalWork(): Out of virtual memory.", 10);
	
				node->head = s->chunk_size;
				node->work = iw_buff;
				iw_buff    = NULL;
	
				ctrk_get(comm_rank, node->work);
	
				/* Push stolen work onto the back of the queue */
				s->nSteal++;    
				s->localWork += s->chunk_size;
				deq_pushBack(localQueue, node);
#ifdef TRACE
				/* Successful Steal */
				ss_markSteal(s, status.MPI_SOURCE);
#endif
			}
			else {
				// Received "No Work" message
				++ctrl_recvd;
			}
	
			// Clear on the outgoing work_request
			MPI_Wait(&wrout_request, &status);
		}
	
		/* Test if we have the token */
		MPI_Test(&td_request, &flag, &status);

		if (flag) {
			enum colors next_token;
			int         forward_token = 1;
		
			DEBUG(DBG_TOKEN, printf("ensureLocalWork(): Thread %d received %s token\n", comm_rank, color_names[td_token.color]));
			switch (td_token.color) {
				case WHITE:
					if (s->localWork == 0) {
						if (comm_rank == 0 && my_color == WHITE) {
							if (td_token.recv_count != td_token.send_count) {
							// There are outstanding messages, try again
								DEBUG(DBG_MSGCNT, printf(" TD_RING: In-flight work, recirculating token\n"));
								my_color = WHITE;
								next_token = WHITE;
							} else {
							// Termination detected, pass RED token
								my_color = PINK;
								next_token = PINK;
							}
						} else if (my_color == WHITE) {
							next_token = WHITE;
						} else {
							// Every time we forward the token, we change
							// our color back to white
							my_color = WHITE;
							next_token = BLACK;
						}

						// forward message
						forward_token = 1;
					}
					else {
						forward_token = 0;
					}
					break;
				case PINK:
					if (comm_rank == 0) {
						if (td_token.recv_count != td_token.send_count) {
							// There are outstanding messages, try again
							DEBUG(DBG_MSGCNT, printf(" TD_RING: ReCirculating pink token nr=%ld ns=%ld\n", td_token.recv_count, td_token.send_count));
							my_color = PINK;
							next_token = PINK;
						} else {
							// Termination detected, pass RED token
							my_color = RED;
							next_token = RED;
						}
					} else {
						my_color = PINK;
						next_token = PINK;
					}

					forward_token = 1;
					break;
				case BLACK:
					// Non-Termination: Token must be recirculated
					if (comm_rank == 0)
						next_token = WHITE;
					else {
						my_color = WHITE;
						next_token = BLACK;
					}
					forward_token = 1;
					break;
				case RED:
					// Termination: Set our state to RED and circulate term message
					my_color = RED;
					next_token = RED;

					if (comm_rank == comm_size - 1)
						forward_token = 0;
					else
						forward_token = 1;
					break;
			}

			/* Forward the token to the next node in the ring */
			if (forward_token) {
				td_token.color = next_token;

				/* Update token counters */
				if (comm_rank == 0) {
					if (td_token.color == PINK) {
						td_token.send_count = ctrl_sent;
						td_token.recv_count = ctrl_recvd;
					} else {
						td_token.send_count = chunks_sent;
						td_token.recv_count = chunks_recvd;
					}
				} else {
					if (td_token.color == PINK) {
						td_token.send_count += ctrl_sent;
						td_token.recv_count += ctrl_recvd;
					} else {
						td_token.send_count += chunks_sent;
						td_token.recv_count += chunks_recvd;
					}
				}

				DEBUG(DBG_TOKEN, printf("ensureLocalWork(): Thread %d forwarding %s token\n", comm_rank, color_names[td_token.color]));
				MPI_Send(&td_token, sizeof(td_token_t), MPI_BYTE, (comm_rank+1)%comm_size,
					MPIWS_TDTOKEN, MPI_COMM_WORLD);

				if (my_color != RED) {
					/* re-Post termination detection listener */
					int j = (comm_rank == 0) ? comm_size - 1 : comm_rank - 1; // Receive the token from the processor to your left
					MPI_Irecv(&td_token, sizeof(td_token_t), MPI_BYTE, j, MPIWS_TDTOKEN, MPI_COMM_WORLD, &td_request);
				}
			}

			if (my_color == RED) {
				// Clean up outstanding requests.
				// This is safe now that the pink token has mopped up all outstanding messages.
				MPI_Cancel(&wrin_request);
				if (iw_request != MPI_REQUEST_NULL)
					MPI_Cancel(&iw_request);
				// Terminate
				return -1;
			}
		}
		
	
	}

	return 0;  // Local work exists
}


/* restore stack to empty state */
void mkEmpty(StealStack *s)
{
	deq_mkEmpty(localQueue);
	s->localWork = 0;
}


/* initialize the stack */
StealStack* ss_init(int *argc, char ***argv)
{
	StealStack* s = &stealStack;

	MPI_Init(argc, argv);

	MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);

        s->globalWork = 0;
        s->localWork  = 0;

	s->nNodes     = 0;
        s->nLeaves    = 0;
	s->nAcquire   = 0;
	s->nRelease   = 0;
	s->nSteal     = 0;
	s->nFail      = 0;

        s->maxStackDepth = 0;
        s->maxTreeDepth  = 0;

	localQueue = deq_create();
	mkEmpty(s);

	// Set a default polling interval
	polling_interval = pollint_default;

	return s;
}

int ss_start(int work_size, int chunk_size)
{
	int j;
	StealStack* s = &stealStack;
	
	s->work_size = work_size;
	s->chunk_size = chunk_size;

	// Start searching for work at the next thread to our right
	wrout_request = MPI_REQUEST_NULL;
	wrout_buff    = chunk_size;
	last_steal    = comm_rank;
	iw_request    = MPI_REQUEST_NULL;
	iw_buff       = NULL; // Allocated on demand
	chunks_sent   = 0;
	chunks_recvd  = 0;
	ctrl_sent     = 0;
	ctrl_recvd    = 0;

	// Using adaptive polling interval?
	if (polling_interval == 0) {
		pollint_isadaptive = 1;
                polling_interval   = 1;
        }

	// Termination detection
	my_color       = WHITE;
	td_token.color = BLACK;

	// Setup non-blocking recieve for recieving shared work requests
	MPI_Irecv(&wrin_buff, 1, MPI_LONG, MPI_ANY_SOURCE, MPIWS_WORKREQUEST, MPI_COMM_WORLD, &wrin_request);

	/* Set up the termination detection receives */	
	if (comm_rank == 0) {
		// Thread 0 initially has a black token
		td_request = MPI_REQUEST_NULL;
	} else {
		/* Post termination detection listener */
		j = (comm_rank == 0) ? comm_size - 1 : comm_rank - 1; // Receive the token from the processor to your left

		MPI_Irecv(&td_token, sizeof(td_token_t), MPI_BYTE, j, MPIWS_TDTOKEN, MPI_COMM_WORLD,
			&td_request);
	}

	return 1;
}

void ss_stop()
{
	DEBUG(DBG_MSGCNT, printf(" Thread %3d: chunks_sent=%ld, chunks_recvd=%ld\n", comm_rank, chunks_sent, chunks_recvd));
	DEBUG(DBG_MSGCNT, printf(" Thread %3d: ctrl_sent  =%ld, ctrl_recvd  =%ld\n", comm_rank, ctrl_sent, ctrl_recvd));

	return;
}

void ss_finalize()
{
	MPI_Finalize();
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
	s->maxStackDepth = max(s->localWork, s->maxStackDepth);
	
	/* If there is sufficient local work, release a chunk to the global queue */
	if (s->nNodes % polling_interval == 0) {
#ifndef TRACE
		ss_setState(s, SS_OVH);
#endif
		ws_make_progress(s);
#ifndef TRACE
		ss_setState(s, SS_WORK);
#endif
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
	//int victimId;
	StealStackNode* n;

	/* Call ensureLocalWork() to make sure there is work on our local queue.
	 * If the local queue is empty, this will get work from the global queue */
	if (ensureLocalWork(s) == -1) {
		DEBUG(DBG_GEN, printf("StealStack::pop - stack is empty and no work can be found\n");fflush(NULL););
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
		MPI_Send(&stealStack, sizeof(StealStack), MPI_BYTE, 0, MPIWS_STATS, MPI_COMM_WORLD);
		return 0;
	} else {
		memcpy(s, &stealStack, sizeof(StealStack));
		for(i = 1; i < *count; i++) {
			MPI_Recv(&(s[i]), sizeof(StealStack), MPI_BYTE, i, MPIWS_STATS, MPI_COMM_WORLD, &status);
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
