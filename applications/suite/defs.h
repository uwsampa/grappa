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


/* Structures */

/* graph structure with explicit start and end vertices for each edge */
typedef struct 
{ int numEdges;
	int * startVertex;	/* sorted, primary key   */
	int * endVertex;	/* sorted, secondary key */
	int * intWeight;	/* integer weight        */ 
	size_t map_size;
} graphSDG;

/* primary graph structure for the kernels */
typedef struct
{ int numEdges;
	int numVertices;
	int * startVertex;    /* start vertex of edge, sorted, primary key      */
	int * endVertex;      /* end   vertex of edge, sorted, secondary key    */
	int * intWeight;      /* integer weight                                 */
	int * edgeStart;      /* index into startVertex and endVertex list      */
	int * marks;		/* array for marking/coloring of vertices	  */
	size_t map_size;
} graph;

#endif
