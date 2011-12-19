// Originally from GraphCT, version 0.6.0
// License: GeorgiaTech

#include "defs.h"

double A, B, C, D;
int SCALE;
vert_id numVertices;
edge_id numEdges;
weight_t maxWeight;
int K4approx;
int subGraphPathLength;

void setupParams(int scale, int edgefactor) {
	/* SCALE */                             /* Binary scaling heuristic  */
	A                  = 0.55;              /* RMAT parameters           */
	B                  = 0.1;
	C                  = 0.1;
	D                  = 0.25;
	numVertices        =  (1 << scale);     /* Total num of vertices     */
	numEdges           =  edgefactor * numVertices;  /* Total num of edges        */
	maxWeight          =  (1 << scale);     /* Max integer weight        */
	K4approx           =  8;                /* Scale factor for K4       */
	subGraphPathLength =  3;                /* Max path length for K3    */
}

