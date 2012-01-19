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

#define DFMT "lld"

/*### Typedefs ###*/
typedef int64_t graphint;
typedef int64_t color_t;

/*### Structures ###*/

/* graph structure with explicit start and end vertices for each edge */
typedef struct {
	graphint numEdges;
	graphint * startVertex;	/* sorted, primary key   */
	graphint * endVertex;	/* sorted, secondary key */
	graphint * intWeight;	/* integer weight        */ 
	size_t map_size;
} graphedges;

/* primary graph structure for the kernels */
typedef struct { 
	graphint numEdges;
	graphint numVertices;
	graphint * startVertex;    /* start vertex of edge, sorted, primary key      */
	graphint * endVertex;      /* end   vertex of edge, sorted, secondary key    */
	graphint * intWeight;      /* integer weight                                 */
	graphint * edgeStart;      /* index into startVertex and endVertex list      */
	color_t * marks;		/* array for marking/coloring of vertices	  */
	size_t map_size;
} graph;

/*### Global Variables ###*/

extern double A;
extern double B;
extern double C;
extern double D;
extern int SCALE;
extern graphint numVertices;
extern graphint numEdges;
extern graphint maxWeight;
extern int K4approx;
extern graphint subGraphPathLength;

/*### Prototypes ###*/

/* timer.c */
double timer(void);

/* xmalloc.c */
void *xmalloc(size_t);
void *xcalloc(size_t, size_t);
void *xrealloc(void*, size_t);
void *xmmap(void*, size_t, int, int, int, off_t);

/* graph-manip.c */
void alloc_graph(graph* g, graphint NV, graphint NE);
void free_graph(graph* g);
void alloc_edgelist(graphedges* g, graphint NE);
void free_edgelist(graphedges* g);

void print_edgelist(graphedges* g, FILE* f);
void print_edgelist_dot(graphedges* g, char* fbase);
void print_graph_dot(graph* g, char* fbase);

/* globals.c */
void setupParams(int scale, int edgefactor);

/* genScalData.c */
void genScalData(graphedges* ing, double a, double b, double c, double d);

/* computeGraph.c */
graph* computeGraph(graphedges* ge);
graph* makeUndirected(graph *G);

/* connectedComponents.c */
graphint connectedComponents(graph *g);

/* pathIsomorphism.c */
#define END -1
graphint pathIsomorphism(graph *g, color_t *pattern, graphint **matches);
void randomizeColors(graph *g, color_t minc, color_t maxc);
void print_match(graph *dirg, color_t *pattern, graphint startVertex);

/* triangles.c */
graphint triangles(graph *g);

/* centrality.c */
double centrality(graph *g, double *bc, graphint vs);

/* util.c */
void printArray(char * title, graphint * array, graphint size);

#endif
