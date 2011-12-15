//
//  defs.h
//  AppSuite
//
//  Created by Brandon Holt on 12/13/11.
//  Copyright 2011 University of Washington. All rights reserved.
//

#ifndef suite_defs_h
#define suite_defs_h

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#if defined(__MTA__)
#include <sys/mta_task.h>
#include <machine/runtime.h>
#elif defined(__MACH__)
#include <mach/mach_time.h>
#else
#include <time.h>
#endif

/*### Macros ###*/

/* Set up MTA() macro that optionally issues mta pragmas if built on the XMT */
#if defined(__MTA__)
#define int64_fetch_add int_fetch_add
#define MTA(x) _Pragma(x)
#if defined(MTA_STREAMS)
#define MTASTREAMS() MTA(MTA_STREAMS)
#else
#define MTASTREAMS() MTA("mta use 100 streams")
#endif
#else
#define MTA(x)
#define MTASTREAMS()
#endif

/*### Prototypes ###*/

/* timer.c */
double timer();

void *xmalloc (size_t);
void *xcalloc (size_t, size_t);
void *xrealloc (void *, size_t);
void *xmmap (void *, size_t, int, int, int, off_t);

/*### Structures ###*/

/* graph structure with explicit start and end vertices for each edge */
struct graphSDG {
	int numEdges;
	int * startVertex;	/* sorted, primary key   */
	int * endVertex;	/* sorted, secondary key */
	int * intWeight;	/* integer weight        */ 
	size_t map_size;
	
	graphSDG(int NE);
	~graphSDG();
};

/* primary graph structure for the kernels */
struct graph { 
	int numEdges;
	int numVertices;
	int * startVertex;    /* start vertex of edge, sorted, primary key      */
	int * endVertex;      /* end   vertex of edge, sorted, secondary key    */
	int * intWeight;      /* integer weight                                 */
	int * edgeStart;      /* index into startVertex and endVertex list      */
	int * marks;		/* array for marking/coloring of vertices	  */
	size_t map_size;
	
	graph(int NV, int NE);
	~graph();
};

#endif
