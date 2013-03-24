#ifndef _DEFS_H
#define _DEFS_H

#ifndef SSCA2
#include <sys/types.h>
typedef int64_t graphint;
#define DFMT "lld"
/* xmalloc.c */
void *
xmalloc(size_t);
void *xcalloc(size_t, size_t);
void *xrealloc(void*, size_t);
void *xmmap(void*, size_t, int, int, int, off_t);
#define MTA(x) _Pragma(x)
#endif

typedef struct 
{ int numEdges;
  int * startVertex;	/* sorted, primary key   */
  int * endVertex;	/* sorted, secondary key */
  int * intWeight;	/* integer weight        */ 
#ifndef SSCA2
  size_t map_size;
#endif
} graphSDG;

typedef struct /* the graph data structure */
{ int numEdges;
  int numVertices;
  int * startVertex;    /* start vertex of edge, sorted, primary key      */
  int * endVertex;      /* end   vertex of edge, sorted, secondary key    */
  int * intWeight;      /* integer weight                                 */
  int * edgeStart;      /* index into startVertex and endVertex list      */
#ifndef SSCA2
  void *marks; /*unused*/
  size_t map_size;
#endif
} graph;

void genScalData(graphSDG*);
void computeGraph(graph*, graphSDG*);
void getStartLists(graph*, int*, int*);
void findSubGraphs(int, int*, graph*);
double centrality(graph*, double*, int);
void getUserParameters(int scale);
void SortStart(int, int, int*, int*, int*, int*, int*);

#endif
