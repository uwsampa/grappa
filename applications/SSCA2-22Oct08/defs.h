#ifndef _DEFS_H
#define _DEFS_H

typedef struct 
{ int numEdges;
  int * startVertex;	/* sorted, primary key   */
  int * endVertex;	/* sorted, secondary key */
  int * intWeight;	/* integer weight        */ 
} graphSDG;

typedef struct /* the graph data structure */
{ int numEdges;
  int numVertices;
  int * startVertex;    /* start vertex of edge, sorted, primary key      */
  int * endVertex;      /* end   vertex of edge, sorted, secondary key    */
  int * intWeight;      /* integer weight                                 */
  int * edgeStart;      /* index into startVertex and endVertex list      */
} graph;

void genScalData(graphSDG*);
void computeGraph(graph*, graphSDG*);
void getStartLists(graph*, int*, int*);
void findSubGraphs(int, int*, graph*);
double centrality(graph*, double*, int);
void getUserParameters(int scale);
void SortStart(int, int, int*, int*, int*, int*, int*);

#endif
