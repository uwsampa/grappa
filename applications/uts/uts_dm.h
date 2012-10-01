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

#ifndef _UTS_DM_H
#define _UTS_DM_H

#include "uts.h"

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

/**
A StealStackNode is an element in a linked list
Each node contains a "chunk" of work
**/
struct stealStackNode_t {
	unsigned long owner_id; /*unique id of the process who created the work, for return information*/
	unsigned long node_id; /*a number incremented by one each time a new node is created, for return information*/
	int head; /*current "head" of the work array, treated as a stack*/
	void* work;  /*a pointer to the "array" of work that needs done, array is of length chunk_size*/
};
typedef struct stealStackNode_t StealStackNode;

/* Search status */
#define STATUS_HAVEWORK 0
#define STATUS_TERM     1

/* Search states */
#define SS_WORK    0
#define SS_SEARCH  1
#define SS_IDLE    2
#define SS_OVH     3
#define SS_NSTATES 4

#ifdef TRACE
/* session record for session visualization */
struct sessionRecord_t
{
  double startTime, endTime;
};
typedef struct sessionRecord_t SessionRecord;

/* steal record for steal visualization */
struct stealRecord_t
{
  long int nodeCount;           /* count nodes generated during the session */
  int victimThread;             /* thread from which we stole the work  */
};
typedef struct stealRecord_t StealRecord;
#endif

/* data per thread */
struct stealStack_t {
	int globalWork;     /* amount work available for stealing */
	int localWork;      /* amount of local only work*/
	int work_size;      /* size of a work node*/
	int chunk_size;     /* amount of work in a steal stack node, also the granularity of steals*/
	counter_t nNodes, nLeaves, nAcquire, nRelease, nSteal, nFail;  /* stats */

	int maxStackDepth;
	int maxTreeDepth;
	
	//FIXME: These stats will be replaced with the trace code
	double walltime;
	double work_time, search_time, idle_time;
	int idle_sessions;  /* steal perf  */

	double time[SS_NSTATES];    /* Time spent in each state */
	double timeLast;
	int    entries[SS_NSTATES]; /* Num sessions of each state */
	int    curState;

	/* Trace Data */
	double startTime;
#ifdef TRACE
	SessionRecord sessionRecords[SS_NSTATES][50000];   /* session time records */
	StealRecord stealRecords[50000];                   /* steal records */
#endif
};
typedef struct stealStack_t StealStack;


/* API Functions - provided by each parallel implementation */
StealStack* ss_init(int *argc, char ***argv);
int         ss_start(int work_size, int chunk_size);
void        ss_stop();
void        ss_finalize();
void        ss_abort(int error);
void        ss_error(char *str, int error);

int         ss_get_work(StealStack *s, void* node_c);
void        ss_put_work(StealStack *s, void* node_c);

int         ss_gather_stats(StealStack *stackptr, int *count);
int         ss_get_thread_num();
int         ss_get_num_threads();
char*       ss_get_par_description();

/* API Statistics gathering functions */
double wctime();

void ss_initStats  (StealStack *s);
void ss_setState   (StealStack *s, int state);
void ss_printTrace (StealStack *s, int numRecords);
void ss_markSteal  (StealStack *s, int victim);


/***  GLOBAL PARAMETERS  ***/
extern int polling_interval;

#endif /* _UTS_DM_H */
