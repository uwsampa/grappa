// Originally from GraphCT, version 0.6.0
// License: GeorgiaTech

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

#define _DEBUG

#ifdef _DEBUG
#define deprint(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)
#define deprintn(fmt) fprintf(stderr, fmt)
#else
#define deprint(fmt, ...) do {} while (0)
#define deprintn(fmt) do {} while (0)
#endif


/*### Typedefs ###*/
typedef int32_t vert_id;
typedef int32_t edge_id;
typedef int32_t weight_t;

/*### Structures ###*/

/* graph structure with explicit start and end vertices for each edge */
typedef struct {
	int numEdges;
	int * startVertex;	/* sorted, primary key   */
	int * endVertex;	/* sorted, secondary key */
	int * intWeight;	/* integer weight        */ 
	size_t map_size;
} edgelist;

/* primary graph structure for the kernels */
typedef struct { 
	int numEdges;
	int numVertices;
	int * startVertex;    /* start vertex of edge, sorted, primary key      */
	int * endVertex;      /* end   vertex of edge, sorted, secondary key    */
	int * intWeight;      /* integer weight                                 */
	int * edgeStart;      /* index into startVertex and endVertex list      */
	int * marks;		/* array for marking/coloring of vertices	  */
	size_t map_size;
} graph;


/*### Global Variables ###*/

extern double A;
extern double B;
extern double C;
extern double D;
extern int SCALE;
extern int numVertices;
extern int numEdges;
extern int maxWeight;
extern int K4approx;
extern int subGraphPathLength;

/*### Prototypes ###*/

/* timer.c */
double timer(void);

/* xmalloc.c */
void *xmalloc(size_t);
void *xcalloc(size_t, size_t);
void *xrealloc(void *, size_t);
void *xmmap(void *, size_t, int, int, int, off_t);

/* graph-manip.c */
void alloc_graph(graph * G, int NV, int NE);
void free_graph(graph * G);
void alloc_edgelist(edgelist * G, int NE);
void free_edgelist(edgelist * G);

/* globals.c */
void setupParams(int scale, int edgefactor);

/* genScalData.c */
void genScalData(edgelist* SDGdataPtr, double a, double b, double c, double d);

#endif
