// Originally from GraphCT, version 0.6.0
// License: GeorgiaTech

#ifndef suite_defs_h
#define suite_defs_h

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "SoftXMT.hpp"

/*### Macros ###*/
#define DFMT "lld"

/*### Typedefs ###*/
typedef int64_t graphint;
typedef graphint color_t;

/*### Structures ###*/

/* graph structure with explicit start and end vertices for each edge */
struct graphedges {
  graphint numEdges;
  GlobalAddress<graphint> startVertex;	/* sorted, primary key   */
  GlobalAddress<graphint> endVertex;	/* sorted, secondary key */
  GlobalAddress<graphint> intWeight;	/* integer weight        */ 
};

/* primary graph structure for the kernels */
struct graph {
  graphint numEdges;
  graphint numVertices;
  GlobalAddress<graphint> startVertex;    /* start vertex of edge, sorted, primary key      */
  GlobalAddress<graphint> endVertex;      /* end   vertex of edge, sorted, secondary key    */
  GlobalAddress<graphint> intWeight;      /* integer weight                                 */
  GlobalAddress<graphint> edgeStart;      /* index into startVertex and endVertex list      */
  GlobalAddress<color_t> marks;           /* array for marking/coloring of vertices	  */
};

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

/* timer.cpp */
double timer();

/* graphManip.cpp */
void alloc_graph(graph* g, graphint NV, graphint NE);
void free_graph(graph* g);
void alloc_edgelist(graphedges* g, graphint NE);
void free_edgelist(graphedges* g);

/* main.cpp */
void setupParams(int scale, int edgefactor);

/* genScalData.cpp */
double genScalData(graphedges* ing, double a, double b, double c, double d);

/* computeGraph.cpp */
void computeGraph(graphedges* ge, graph * dirg);
void makeUndirected(graph *dirg, graph * g);

/* connectedComponents.cpp */
graphint connectedComponents(graph *g);

/* pathIsomorphism.cpp */
graphint pathIsomorphism(graph *g, color_t* pattern, size_t npattern);

void markColors(graph *g, color_t minc, color_t maxc);
void print_match(graph *dirg, color_t *pattern, graphint startVertex);

/* triangles.cpp */
graphint triangles(graph *g);

/* centrality.cpp */
double centrality(graph *g, GlobalAddress<double> bc, graphint vs);

#endif
